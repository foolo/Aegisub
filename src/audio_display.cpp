// Copyright (c) 2005, Rodrigo Braz Monteiro
// Copyright (c) 2009-2010, Niels Martin Hansen
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Aegisub Project http://www.aegisub.org/

#include "audio_display.h"

#include "audio_controller.h"
#include "audio_renderer.h"
#include "audio_renderer_spectrum.h"
#include "audio_renderer_waveform.h"
#include "audio_timing.h"
#include "compat.h"
#include "format.h"
#include "include/aegisub/context.h"
#include "include/aegisub/hotkey.h"
#include "options.h"
#include "project.h"
#include "utils.h"
#include "video_controller.h"
#include "ass_file.h"
#include "ass_dialogue.h"
#include "selection_controller.h"

#include <libaegisub/ass/time.h>
#include <libaegisub/audio/provider.h>
#include <libaegisub/make_unique.h>

#include <algorithm>

#include <wx/dcbuffer.h>
#include <wx/mousestate.h>
#include <wx/graphics.h>

AudioDisplayTimeline::AudioDisplayTimeline(AudioDisplay *display)
: display(display)
{
	int width, height;
	display->GetTextExtent("0123456789:.", &width, &height);
	bounds.height = height + 4;
}

void AudioDisplayTimeline::SetDisplaySize(const wxSize &display_size)
{
	// The size is without anything that goes below the timeline (like scrollbar)
	bounds.width = display_size.x;
	bounds.x = 0;
	bounds.y = 0;
}

void AudioDisplayTimeline::ChangeAudio(int new_duration)
{
	duration = new_duration;
}

void AudioDisplayTimeline::ChangeZoom(double new_ms_per_pixel)
{
	ms_per_pixel = new_ms_per_pixel;

	double px_sec = 1000.0 / ms_per_pixel;

	if (px_sec > 3000) {
		scale_minor = Sc_Millisecond;
		scale_minor_divisor = 1.0;
		scale_major_modulo = 10;
	} else if (px_sec > 300) {
		scale_minor = Sc_Centisecond;
		scale_minor_divisor = 10.0;
		scale_major_modulo = 10;
	} else if (px_sec > 30) {
		scale_minor = Sc_Decisecond;
		scale_minor_divisor = 100.0;
		scale_major_modulo = 10;
	} else if (px_sec > 3) {
		scale_minor = Sc_Second;
		scale_minor_divisor = 1000.0;
		scale_major_modulo = 10;
	} else if (px_sec > 1.0/3.0) {
		scale_minor = Sc_Decasecond;
		scale_minor_divisor = 10000.0;
		scale_major_modulo = 6;
	} else if (px_sec > 1.0/9.0) {
		scale_minor = Sc_Minute;
		scale_minor_divisor = 60000.0;
		scale_major_modulo = 10;
	} else if (px_sec > 1.0/90.0) {
		scale_minor = Sc_Decaminute;
		scale_minor_divisor = 600000.0;
		scale_major_modulo = 6;
	} else {
		scale_minor = Sc_Hour;
		scale_minor_divisor = 3600000.0;
		scale_major_modulo = 10;
	}
}

void AudioDisplayTimeline::SetPosition(int new_pixel_left)
{
	pixel_left = std::max(new_pixel_left, 0);
}

bool AudioDisplayTimeline::OnMouseEvent(wxMouseEvent &event)
{
	return false;
}

wxColor FOREGROUND_COLOR = *wxBLACK;
wxColor BACKGROUND_COLOR = *wxWHITE;

void AudioDisplayTimeline::Paint(wxDC &dc)
{
	int bottom = bounds.y + bounds.height;

	// Background
	dc.SetPen(wxPen(BACKGROUND_COLOR));
	dc.SetBrush(wxBrush(BACKGROUND_COLOR));
	dc.DrawRectangle(bounds);

	// Top line
	dc.SetPen(wxPen(FOREGROUND_COLOR));
	dc.DrawLine(bounds.x, bottom-1, bounds.x+bounds.width, bottom-1);

	// Prepare for writing text
	dc.SetTextBackground(BACKGROUND_COLOR);
	dc.SetTextForeground(FOREGROUND_COLOR);

	// Figure out the first scale mark to show
	int ms_left = int(pixel_left * ms_per_pixel);
	int next_scale_mark = int(ms_left / scale_minor_divisor);
	if (next_scale_mark * scale_minor_divisor < ms_left)
		next_scale_mark += 1;
	assert(next_scale_mark * scale_minor_divisor >= ms_left);

	// Draw scale marks
	int next_scale_mark_pos;
	int last_text_right = -1;
	int last_hour = -1, last_minute = -1;
	if (duration < 3600) last_hour = 0; // Trick to only show hours if audio is longer than 1 hour
	do {
		next_scale_mark_pos = int(next_scale_mark * scale_minor_divisor / ms_per_pixel) - pixel_left;
		bool mark_is_major = next_scale_mark % scale_major_modulo == 0;

		if (mark_is_major)
			dc.DrawLine(next_scale_mark_pos, bottom-6, next_scale_mark_pos, bottom-1);
		else
			dc.DrawLine(next_scale_mark_pos, bottom-4, next_scale_mark_pos, bottom-1);

		// Print time labels on major scale marks
		if (mark_is_major && next_scale_mark_pos > last_text_right)
		{
			double mark_time = next_scale_mark * scale_minor_divisor / 1000.0;
			int mark_hour = (int)(mark_time / 3600);
			int mark_minute = (int)(mark_time / 60) % 60;
			double mark_second = mark_time - mark_hour*3600.0 - mark_minute*60.0;

			wxString time_string;
			bool changed_hour = mark_hour != last_hour;
			bool changed_minute = mark_minute != last_minute;

			if (changed_hour)
			{
				time_string = fmt_wx("%d:%02d:", mark_hour, mark_minute);
				last_hour = mark_hour;
				last_minute = mark_minute;
			}
			else if (changed_minute)
			{
				time_string = fmt_wx("%d:", mark_minute);
				last_minute = mark_minute;
			}
			if (scale_minor >= Sc_Decisecond)
				time_string += fmt_wx("%02d", mark_second);
			else if (scale_minor == Sc_Centisecond)
				time_string += fmt_wx("%02.1f", mark_second);
			else
				time_string += fmt_wx("%02.2f", mark_second);

			int tw, th;
			dc.GetTextExtent(time_string, &tw, &th);
			last_text_right = next_scale_mark_pos + tw;

			dc.DrawText(time_string, next_scale_mark_pos, 0);
		}

		next_scale_mark += 1;

	} while (next_scale_mark_pos < bounds.width);
}

class AudioMarkerInteractionObject final : public AudioDisplayInteractionObject {
	// Object-pair being interacted with
	std::vector<AudioMarker*> markers;
	AudioTimingController *timing_controller;
	// Audio display drag is happening on
	AudioDisplay *display;
	// Mouse button used to initiate the drag
	wxMouseButton button_used;
	// Default to snapping to snappable markers
	bool default_snap = OPT_GET("Audio/Snap/Enable")->GetBool();
	// Range in pixels to snap at
	int snap_range = OPT_GET("Audio/Snap/Distance")->GetInt();

public:
	AudioMarkerInteractionObject(std::vector<AudioMarker*> markers, AudioTimingController *timing_controller, AudioDisplay *display, wxMouseButton button_used)
	: markers(std::move(markers))
	, timing_controller(timing_controller)
	, display(display)
	, button_used(button_used)
	{
	}

	bool OnMouseEvent(wxMouseEvent &event) override
	{
		if (event.Dragging())
		{
			timing_controller->OnMarkerDrag(
				markers,
				display->TimeFromRelativeX(event.GetPosition().x),
				default_snap != event.ShiftDown() ? display->TimeFromAbsoluteX(snap_range) : 0);
		}

		// We lose the marker drag if the button used to initiate it goes up
		return !event.ButtonUp(button_used);
	}

	/// Get the position in milliseconds of this group of markers
	int GetPosition() const { return markers.front()->GetPosition(); }
};

AudioDisplay::AudioDisplay(wxWindow *parent, AudioController *controller, agi::Context *context)
: wxWindow(parent, -1, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS|wxBORDER_SIMPLE)
, audio_open_connection(context->project->AddAudioProviderListener(&AudioDisplay::OnAudioOpen, this))
, context(context)
, audio_renderer(agi::make_unique<AudioRenderer>())
, controller(controller)
, timeline(agi::make_unique<AudioDisplayTimeline>(this))
, state(DRAGGING_IDLE)
{
	audio_renderer->SetAmplitudeScale(scale_amplitude);
	SetZoomLevel(0);

	SetMinClientSize(wxSize(-1, 70));
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetThemeEnabled(false);

	Bind(wxEVT_LEFT_DOWN, &AudioDisplay::OnMouseEvent, this);
	Bind(wxEVT_MIDDLE_DOWN, &AudioDisplay::OnMouseEvent, this);
	Bind(wxEVT_RIGHT_DOWN, &AudioDisplay::OnMouseEvent, this);
	Bind(wxEVT_LEFT_UP, &AudioDisplay::OnMouseEvent, this);
	Bind(wxEVT_MIDDLE_UP, &AudioDisplay::OnMouseEvent, this);
	Bind(wxEVT_RIGHT_UP, &AudioDisplay::OnMouseEvent, this);
	Bind(wxEVT_MOTION, &AudioDisplay::OnMouseEvent, this);
	Bind(wxEVT_ENTER_WINDOW, &AudioDisplay::OnMouseEnter, this);
	Bind(wxEVT_LEAVE_WINDOW, &AudioDisplay::OnMouseLeave, this);
	Bind(wxEVT_PAINT, &AudioDisplay::OnPaint, this);
	Bind(wxEVT_SIZE, &AudioDisplay::OnSize, this);
	Bind(wxEVT_CHAR_HOOK, &AudioDisplay::OnKeyDown, this);
	Bind(wxEVT_KEY_DOWN, &AudioDisplay::OnKeyDown, this);
	scroll_timer.Bind(wxEVT_TIMER, &AudioDisplay::OnScrollTimer, this);
	load_timer.Bind(wxEVT_TIMER, &AudioDisplay::OnLoadTimer, this);
}

AudioDisplay::~AudioDisplay()
{
}

void AudioDisplay::ScrollBy(int pixel_amount)
{
	ScrollPixelToLeft(scroll_left + pixel_amount);
}

void AudioDisplay::ScrollPixelToLeft(int pixel_position)
{
	const int client_width = GetClientRect().GetWidth();

	if (pixel_position + client_width >= pixel_audio_width)
		pixel_position = pixel_audio_width - client_width;
	if (pixel_position < 0)
		pixel_position = 0;

	scroll_left = pixel_position;
	timeline->SetPosition(scroll_left);
	Refresh();
}

void AudioDisplay::ScrollTimeRangeInView(const TimeRange &range)
{
	int client_width = GetClientRect().GetWidth();
	int range_begin = AbsoluteXFromTime(range.begin());
	int range_end = AbsoluteXFromTime(range.end());
	int range_len = range_end - range_begin;

	// Remove 5 % from each side of the client area.
	int leftadjust = client_width / 20;
	int client_left = scroll_left + leftadjust;
	client_width = client_width * 9 / 10;

	// Is everything already in view?
	if (range_begin >= client_left && range_end <= client_left+client_width)
		return;

	// The entire range can fit inside the view, center it
	if (range_len < client_width)
	{
		ScrollPixelToLeft(range_begin - (client_width-range_len)/2 - leftadjust);
	}

	// Range doesn't fit in view and we're viewing a middle part of it, just leave it alone
	else if (range_begin < client_left && range_end > client_left+client_width)
	{
		// nothing
	}

	// Right edge is in view, scroll it as far to the right as possible
	else if (range_end >= client_left && range_end < client_left+client_width)
	{
		ScrollPixelToLeft(range_end - client_width - leftadjust);
	}

	// Nothing is in view or the left edge is in view, scroll left edge as far to the left as possible
	else
	{
		ScrollPixelToLeft(range_begin - leftadjust);
	}
}

void AudioDisplay::SetZoomLevel(int new_zoom_level)
{
	zoom_level = new_zoom_level;

	const int factor = GetZoomLevelFactor(zoom_level);
	const int base_pixels_per_second = 50; /// @todo Make this customisable
	const double base_ms_per_pixel = 1000.0 / base_pixels_per_second;
	const double new_ms_per_pixel = 100.0 * base_ms_per_pixel / factor;

	if (ms_per_pixel == new_ms_per_pixel) return;

	int client_width = GetClientSize().GetWidth();
	double cursor_pos = track_cursor_pos >= 0 ? track_cursor_pos - scroll_left : client_width / 2.0;
	double cursor_time = (scroll_left + cursor_pos) * ms_per_pixel;

	ms_per_pixel = new_ms_per_pixel;
	pixel_audio_width = std::max(1, int(GetDuration() / ms_per_pixel));

	audio_renderer->SetMillisecondsPerPixel(ms_per_pixel);
	timeline->ChangeZoom(ms_per_pixel);

	ScrollPixelToLeft(AbsoluteXFromTime(cursor_time) - cursor_pos);
	if (track_cursor_pos >= 0)
		track_cursor_pos = AbsoluteXFromTime(cursor_time);
	Refresh();
}

wxString AudioDisplay::GetZoomLevelDescription(int level) const
{
	const int factor = GetZoomLevelFactor(level);
	const int base_pixels_per_second = 50; /// @todo Make this customisable along with the above
	const int second_pixels = 100 * base_pixels_per_second / factor;

	return fmt_tl("%d%%, %d pixel/second", factor, second_pixels);
}

int AudioDisplay::GetZoomLevelFactor(int level)
{
	int factor = 100;

	if (level > 0)
	{
		factor += 25 * level;
	}
	else if (level < 0)
	{
		if (level >= -5)
			factor += 10 * level;
		else if (level >= -11)
			factor = 50 + (level+5) * 5;
		else
			factor = 20 + level + 11;
		if (factor <= 0)
			factor = 1;
	}

	return factor;
}

void AudioDisplay::SetAmplitudeScale(float scale)
{
	audio_renderer->SetAmplitudeScale(scale);
	Refresh();
}

void AudioDisplay::ReloadRenderingSettings()
{
	std::string colour_scheme_name;

	if (OPT_GET("Audio/Spectrum")->GetBool())
	{
		auto audio_spectrum_renderer = agi::make_unique<AudioSpectrumRenderer>();

		int64_t spectrum_quality = OPT_GET("Audio/Renderer/Spectrum/Quality")->GetInt();
#ifdef WITH_FFTW3
		// FFTW is so fast we can afford to upgrade quality by two levels
		spectrum_quality += 2;
#endif
		spectrum_quality = mid<int64_t>(0, spectrum_quality, 5);

		// Quality indexes:        0  1  2  3   4   5
		int spectrum_width[]    = {8, 9, 9, 9, 10, 11};
		int spectrum_distance[] = {8, 8, 7, 6,  6,  5};

		audio_spectrum_renderer->SetResolution(
			spectrum_width[spectrum_quality],
			spectrum_distance[spectrum_quality]);

		audio_renderer_provider = std::move(audio_spectrum_renderer);
	}
	else
	{
		audio_renderer_provider = agi::make_unique<AudioWaveformRenderer>();
	}

	audio_renderer->SetRenderer(audio_renderer_provider.get());

	Refresh();
}

void AudioDisplay::OnLoadTimer(wxTimerEvent&)
{
	using namespace std::chrono;
	if (provider)
	{
		const auto now = steady_clock::now();
		const auto elapsed = duration_cast<milliseconds>(now - audio_load_start_time).count();
		if (elapsed == 0) return;

		const int64_t new_decoded_count = provider->GetDecodedSamples();
		if (new_decoded_count != last_sample_decoded)
			audio_load_speed = (audio_load_speed + (double)new_decoded_count / elapsed) / 2;
		if (audio_load_speed == 0) return;

		int new_pos = AbsoluteXFromTime(elapsed * audio_load_speed * 1000.0 / provider->GetSampleRate());
		if (new_pos > audio_load_position)
			audio_load_position = new_pos;

		const double left = last_sample_decoded * 1000.0 / provider->GetSampleRate() / ms_per_pixel;
		const double right = new_decoded_count * 1000.0 / provider->GetSampleRate() / ms_per_pixel;

		if (left < scroll_left + pixel_audio_width && right >= scroll_left)
			Refresh();
		last_sample_decoded = new_decoded_count;
	}

	if (!provider || last_sample_decoded == provider->GetNumSamples()) {
		load_timer.Stop();
		audio_load_position = -1;
	}
}

const int SUBTITLE_ALPHA = 160;
const wxColour ACTIVE_SUBTITLE_COLOR(128, 255, 128, SUBTITLE_ALPHA);
const wxColour SELECTED_SUBITLE_COLOR(192, 255, 192, SUBTITLE_ALPHA);
const wxColour INACTIVE_SUBTITLE_COLOR(255, 255, 255, SUBTITLE_ALPHA);

void AudioDisplay::OnPaint(wxPaintEvent&)
{
	if (!audio_renderer_provider || !provider) return;

	wxAutoBufferedPaintDC dc(this);

	wxRect audio_bounds(0, audio_top, GetClientSize().GetWidth(), audio_height);
	bool redraw_timeline = false;

	for (wxRegionIterator region(GetUpdateRegion()); region; ++region)
	{
		wxRect updrect = region.GetRect();

		redraw_timeline |= timeline->GetBounds().Intersects(updrect);

		if (audio_bounds.Intersects(updrect))
		{
			TimeRange updtime(
				std::max(0, TimeFromRelativeX(updrect.x - foot_size)),
				std::max(0, TimeFromRelativeX(updrect.x + updrect.width + foot_size)));

			PaintAudio(dc, updtime, updrect);
			PaintMarkers(dc, updtime);
			PaintLabels(dc, updtime);
		}
	}

	wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
	int start_time = TimeFromRelativeX(0 - foot_size);
	int end_time = TimeFromRelativeX(GetClientSize().GetWidth() + foot_size);

	gc->SetPen(*wxWHITE);
	for (AssDialogue& line : context->ass->Events) {
		bool off_screen = (line.Start > end_time) || (line.End < start_time);
		if (off_screen) {
			continue;
		}

		if (&line == context->selectionController->GetActiveLine()) {
			gc->SetBrush(ACTIVE_SUBTITLE_COLOR);
		}
		else if (context->selectionController->IsSelected(&line)) {
			gc->SetBrush(SELECTED_SUBITLE_COLOR);
		}
		else {
			gc->SetBrush(INACTIVE_SUBTITLE_COLOR);
		}

		int x1 = RelativeXFromTime(line.Start);
		int x2 = RelativeXFromTime(line.End);
		gc->DrawRoundedRectangle(x1, audio_top, x2-x1, audio_height, 5);
	}

	if (redraw_timeline)
		timeline->Paint(dc);

	if (track_cursor_pos >= 0)
		PaintTrackCursor(dc);
}

void AudioDisplay::PaintAudio(wxDC &dc, const TimeRange updtime, const wxRect updrect)
{
	const int range_x1 = updrect.x;
	int range_x2 = updrect.x + updrect.width;
	if (range_x2 > range_x1) {
		audio_renderer->Render(dc, wxPoint(range_x1, audio_top), range_x1 + scroll_left, range_x2 - range_x1);
	}
}

void AudioDisplay::PaintMarkers(wxDC &dc, TimeRange updtime)
{
	AudioMarkerVector markers;
	controller->GetTimingController()->GetMarkers(updtime, markers);
	if (markers.empty()) return;

	wxDCPenChanger pen_retainer(dc, wxPen());
	wxDCBrushChanger brush_retainer(dc, wxBrush());
	for (const auto marker : markers)
	{
		int marker_x = RelativeXFromTime(marker->GetPosition());

		dc.SetPen(marker->GetStyle());
		dc.DrawLine(marker_x, audio_top, marker_x, audio_top+audio_height);

		if (marker->GetFeet() == AudioMarker::Feet_None) continue;

		dc.SetBrush(wxBrush(marker->GetStyle().GetColour()));
		dc.SetPen(*wxTRANSPARENT_PEN);

		if (marker->GetFeet() & AudioMarker::Feet_Left)
			PaintFoot(dc, marker_x, -1);
		if (marker->GetFeet() & AudioMarker::Feet_Right)
			PaintFoot(dc, marker_x, 1);
	}
}

void AudioDisplay::PaintFoot(wxDC &dc, int marker_x, int dir)
{
	wxPoint foot_top[3] = { wxPoint(foot_size * dir, 0), wxPoint(0, 0), wxPoint(0, foot_size) };
	wxPoint foot_bot[3] = { wxPoint(foot_size * dir, 0), wxPoint(0, -foot_size), wxPoint(0, 0) };
	dc.DrawPolygon(3, foot_top, marker_x, audio_top);
	dc.DrawPolygon(3, foot_bot, marker_x, audio_top+audio_height);
}

void AudioDisplay::PaintLabels(wxDC &dc, TimeRange updtime)
{
	std::vector<AudioLabelProvider::AudioLabel> labels;
	controller->GetTimingController()->GetLabels(updtime, labels);
	if (labels.empty()) return;

	wxDCFontChanger fc(dc);
	wxFont font = dc.GetFont();
	font.SetWeight(wxFONTWEIGHT_BOLD);
	fc.Set(font);
	dc.SetTextForeground(*wxWHITE);
	for (auto const& label : labels)
	{
		wxSize extent = dc.GetTextExtent(label.text);
		int left = RelativeXFromTime(label.range.begin());
		int width = AbsoluteXFromTime(label.range.length());

		// If it doesn't fit, truncate
		if (width < extent.GetWidth())
		{
			dc.SetClippingRegion(left, audio_top + 4, width, extent.GetHeight());
			dc.DrawText(label.text, left, audio_top + 4);
			dc.DestroyClippingRegion();
		}
		// Otherwise center in the range
		else
		{
			dc.DrawText(label.text, left + (width - extent.GetWidth()) / 2, audio_top + 4);
		}
	}
}


void AudioDisplay::PaintTrackCursor(wxDC &dc) {
	wxDCPenChanger penchanger(dc, wxPen(*wxWHITE));
	dc.DrawLine(track_cursor_pos-scroll_left, 0, track_cursor_pos-scroll_left, GetClientSize().GetHeight());

	if (track_cursor_label.empty()) return;

	wxDCFontChanger fc(dc);
	wxFont font = dc.GetFont();
	font.SetWeight(wxFONTWEIGHT_BOLD);
	fc.Set(font);

	wxSize label_size(dc.GetTextExtent(track_cursor_label));
	wxPoint label_pos(track_cursor_pos - scroll_left - label_size.x/2, audio_top + 2);
	label_pos.x = mid(2, label_pos.x, GetClientSize().GetWidth() - label_size.x - 2);

	int old_bg_mode = dc.GetBackgroundMode();
	dc.SetBackgroundMode(wxTRANSPARENT);

	// Draw border
	dc.SetTextForeground(wxColour(64, 64, 64));
	dc.DrawText(track_cursor_label, label_pos.x+1, label_pos.y+1);
	dc.DrawText(track_cursor_label, label_pos.x+1, label_pos.y-1);
	dc.DrawText(track_cursor_label, label_pos.x-1, label_pos.y+1);
	dc.DrawText(track_cursor_label, label_pos.x-1, label_pos.y-1);

	// Draw fill
	dc.SetTextForeground(*wxWHITE);
	dc.DrawText(track_cursor_label, label_pos.x, label_pos.y);
	dc.SetBackgroundMode(old_bg_mode);

	label_pos.x -= 2;
	label_pos.y -= 2;
	label_size.IncBy(4, 4);
	// If the rendered text changes size we have to draw it an extra time to make sure the entire thing was drawn
	bool need_extra_redraw = track_cursor_label_rect.GetSize() != label_size;
	track_cursor_label_rect.SetPosition(label_pos);
	track_cursor_label_rect.SetSize(label_size);
	if (need_extra_redraw)
		RefreshRect(track_cursor_label_rect, false);
}

void AudioDisplay::SetTrackCursor(int new_pos, bool show_time)
{
	if (new_pos == track_cursor_pos) return;

	int old_pos = track_cursor_pos;
	track_cursor_pos = new_pos;

	int client_height = GetClientSize().GetHeight();
	RefreshRect(wxRect(old_pos - scroll_left - 1, 0, 2, client_height), false);
	RefreshRect(wxRect(new_pos - scroll_left - 1, 0, 2, client_height), false);

	// Make sure the old label gets cleared away
	RefreshRect(track_cursor_label_rect, false);

	if (show_time)
	{
		agi::Time new_label_time = TimeFromAbsoluteX(track_cursor_pos);
		track_cursor_label = to_wx(new_label_time.GetAssFormatted());
		track_cursor_label_rect.x += new_pos - old_pos;
		RefreshRect(track_cursor_label_rect, false);
	}
	else
	{
		track_cursor_label_rect.SetSize(wxSize(0,0));
		track_cursor_label.Clear();
	}
}

void AudioDisplay::RemoveTrackCursor()
{
	SetTrackCursor(-1, false);
}

void AudioDisplay::JumpToTime(int mouse_x) {
	context->videoController->JumpToTime(TimeFromRelativeX(mouse_x), agi::vfr::EXACT);
	SetTrackCursor(scroll_left + mouse_x, OPT_GET("Audio/Display/Draw/Cursor Time")->GetBool());
}

void AudioDisplay::OnMouseEnter(wxMouseEvent&)
{
	if (OPT_GET("Audio/Auto/Focus")->GetBool())
		SetFocus();
}

void AudioDisplay::OnMouseLeave(wxMouseEvent&)
{
	if (!controller->IsPlaying())
		RemoveTrackCursor();
}


void AudioDisplay::OnMouseEvent(wxMouseEvent& event)
{
	// If we have focus, we get mouse move events on Mac even when the mouse is
	// outside our client rectangle, we don't want those.
	if (event.Moving() && !GetClientRect().Contains(event.GetPosition()))
	{
		event.Skip();
		return;
	}

	if (event.IsButton())
		SetFocus();

	const int mouse_x = event.GetPosition().x;

	const wxPoint mousepos = event.GetPosition();

	DragState newState = state;
	if ((state != DRAGGING_IDLE) && (HasCapture() == false)) {
		// error handling, capture lost
		newState = DRAGGING_IDLE;
		SetCursor(wxNullCursor);
	}
	else if (state == DRAGGING_IDLE) {
		if (timeline->GetBounds().Contains(mousepos)) {
			if (event.LeftDown()) {
				JumpToTime(mouse_x);
				newState = DRAGGING_TIMELINE;
			}
		}
		else {
			AudioTimingController *timing = controller->GetTimingController();
			if (!timing) return;
			const int drag_sensitivity = int(OPT_GET("Audio/Start Drag Sensitivity")->GetInt() * ms_per_pixel);
			const int snap_sensitivity = OPT_GET("Audio/Snap/Enable")->GetBool() != event.ShiftDown() ? int(OPT_GET("Audio/Snap/Distance")->GetInt() * ms_per_pixel) : 0;

			// Not scrollbar, not timeline, no button action
			if (event.Moving())
			{
				const int timepos = TimeFromRelativeX(mouse_x);

				if (timing->IsNearbyMarker(timepos, drag_sensitivity, event.AltDown()))
					SetCursor(wxCursor(wxCURSOR_SIZEWE));
				else
					SetCursor(wxNullCursor);
				return;
			}

			const int old_scroll_pos = scroll_left;
			if (event.LeftDown() || event.RightDown())
			{
				const int timepos = TimeFromRelativeX(mouse_x);
				std::vector<AudioMarker*> markers = event.LeftDown()
					? timing->OnLeftClick(timepos, event.CmdDown(), event.AltDown(), drag_sensitivity, snap_sensitivity)
					: timing->OnRightClick(timepos, event.CmdDown(), drag_sensitivity, snap_sensitivity);

				// Clicking should never result in the audio display scrolling
				ScrollPixelToLeft(old_scroll_pos);

				if (markers.size())
				{
					RemoveTrackCursor();
					audio_marker = agi::make_unique<AudioMarkerInteractionObject>(markers, timing, this, (wxMouseButton)event.GetButton());
					newState = DRAGGING_AUDIO_MARKER;
				}
			}
		}
	}
	else if (state == DRAGGING_TIMELINE) {
		JumpToTime(mouse_x);
		if (event.LeftIsDown() == false) {
			newState = DRAGGING_IDLE;
		}
	}
	else if (state == DRAGGING_AUDIO_MARKER) {
		if (!audio_marker->OnMouseEvent(event)) {
			scroll_timer.Stop();
			newState = DRAGGING_IDLE;
			SetCursor(wxNullCursor);
		}
	}

	state = newState;

	if (state != DRAGGING_IDLE) {
		if (HasCapture() == false) {
			CaptureMouse();
		}
		return;
	}

	if (HasCapture()) {
		ReleaseMouse();
	}

	if (event.MiddleIsDown()) {
		JumpToTime(mouse_x);
		return;
	}

	if (!controller->IsPlaying()) {
		RemoveTrackCursor();
	}
}

void AudioDisplay::OnKeyDown(wxKeyEvent& event)
{
	hotkey::check("Audio", context, event);
}

void AudioDisplay::OnSize(wxSizeEvent &)
{
	// We changed size, update the sub-controls' internal data and redraw
	wxSize size = GetClientSize();

	timeline->SetDisplaySize(wxSize(size.x, 0));

	audio_height = size.GetHeight();
	audio_height -= timeline->GetHeight();
	audio_renderer->SetHeight(audio_height);

	audio_top = timeline->GetHeight();

	Refresh();
}

int AudioDisplay::GetDuration() const
{
	if (!provider) return 0;
	return (provider->GetNumSamples() * 1000 + provider->GetSampleRate() - 1) / provider->GetSampleRate();
}

void AudioDisplay::OnAudioOpen(agi::AudioProvider *provider)
{
	this->provider = provider;

	if (!audio_renderer_provider)
		ReloadRenderingSettings();

	audio_renderer->SetAudioProvider(provider);
	audio_renderer->SetCacheMaxSize(OPT_GET("Audio/Renderer/Spectrum/Memory Max")->GetInt() * 1024 * 1024);

	timeline->ChangeAudio(GetDuration());

	ms_per_pixel = 0;
	SetZoomLevel(zoom_level);

	Refresh();

	if (provider)
	{
		if (connections.empty())
		{
			connections = agi::signal::make_vector({
				controller->AddPlaybackPositionListener(&AudioDisplay::OnPlaybackPosition, this),
				controller->AddPlaybackStopListener(&AudioDisplay::RemoveTrackCursor, this),
				controller->AddTimingControllerListener(&AudioDisplay::OnTimingController, this),
				OPT_SUB("Audio/Spectrum", &AudioDisplay::ReloadRenderingSettings, this),
				OPT_SUB("Audio/Display/Waveform Style", &AudioDisplay::ReloadRenderingSettings, this),
				OPT_SUB("Colour/Audio Display/Spectrum", &AudioDisplay::ReloadRenderingSettings, this),
				OPT_SUB("Colour/Audio Display/Waveform", &AudioDisplay::ReloadRenderingSettings, this),
				OPT_SUB("Audio/Renderer/Spectrum/Quality", &AudioDisplay::ReloadRenderingSettings, this),
			});
			OnTimingController();
		}

		last_sample_decoded = provider->GetDecodedSamples();
		audio_load_position = -1;
		audio_load_speed = 0;
		audio_load_start_time = std::chrono::steady_clock::now();
		if (last_sample_decoded != provider->GetNumSamples())
			load_timer.Start(100);
	}
	else
	{
		connections.clear();
	}
}

void AudioDisplay::OnTimingController()
{
	AudioTimingController *timing_controller = controller->GetTimingController();
	if (timing_controller)
	{
		timing_controller->AddMarkerMovedListener(&AudioDisplay::OnMarkerMoved, this);
		timing_controller->AddUpdatedPrimaryRangeListener(&AudioDisplay::OnSelectionChanged, this);

		OnMarkerMoved();
		OnSelectionChanged();
	}
}

void AudioDisplay::OnPlaybackPosition(int ms)
{
	int pixel_position = AbsoluteXFromTime(ms);
	SetTrackCursor(pixel_position, false);

	if (OPT_GET("Audio/Lock Scroll on Cursor")->GetBool())
	{
		int client_width = GetClientSize().GetWidth();
		int edge_size = client_width / 20;
		if (scroll_left > 0 && pixel_position < scroll_left + edge_size)
		{
			ScrollPixelToLeft(std::max(pixel_position - edge_size, 0));
		}
		else if (scroll_left + client_width < std::min(pixel_audio_width - 1, pixel_position + edge_size))
		{
			if (OPT_GET("Audio/Smooth Scrolling")->GetBool()) {
				ScrollPixelToLeft(std::min(pixel_position - client_width + edge_size, pixel_audio_width - client_width - 1));
			}
			else {
				ScrollPixelToLeft(std::min(pixel_position - edge_size, pixel_audio_width - client_width - 1));
			}
		}
	}
}

void AudioDisplay::OnSelectionChanged()
{
	TimeRange sel(controller->GetPrimaryPlaybackRange());

	if (audio_marker)
	{
		if (!scroll_timer.IsRunning())
		{
			// If the dragged object is outside the visible area, start the
			// scroll timer to shift it back into view
			int rel_x = RelativeXFromTime(audio_marker->GetPosition());
			if (rel_x < 0 || rel_x >= GetClientSize().GetWidth())
			{
				// 50ms is the default for this on Windows (hardcoded since
				// wxSystemSettings doesn't expose DragScrollDelay etc.)
				scroll_timer.Start(50, true);
			}
		}
	}
	else if (OPT_GET("Audio/Auto/Scroll")->GetBool() && sel.end() != 0)
	{
		ScrollTimeRangeInView(sel);
	}
}

void AudioDisplay::OnScrollTimer(wxTimerEvent &event)
{
	if (!audio_marker) return;

	int rel_x = RelativeXFromTime(audio_marker->GetPosition());
	int width = GetClientSize().GetWidth();

	// If the dragged object is outside the visible area, scroll it into
	// view with a 5% margin
	if (rel_x < 0)
	{
		ScrollBy(rel_x - width / 20);
	}
	else if (rel_x >= width)
	{
		ScrollBy(rel_x - width + width / 20);
	}
}

void AudioDisplay::OnMarkerMoved()
{
	RefreshRect(wxRect(0, audio_top, GetClientSize().GetWidth(), audio_height), false);
}
