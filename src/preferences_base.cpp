// Copyright (c) 2010, Amar Takhar <verm@aegisub.org>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

/// @file preferences_base.cpp
/// @brief Base preferences dialogue classes
/// @ingroup configuration_ui

#include "preferences_base.h"

#include "colour_button.h"
#include "compat.h"
#include "options.h"
#include "preferences.h"

#include <libaegisub/exception.h>
#include <libaegisub/path.h>
#include <libaegisub/make_unique.h>

#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/dirdlg.h>
#include <wx/event.h>
#include <wx/fontdlg.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/treebook.h>

#undef Bool

class StringUpdater {
	std::string name;
	Preferences *parent;
public:
	StringUpdater(std::string const& n, Preferences *p) : name(n), parent(p) { }
	void operator()(wxCommandEvent& evt) {
		evt.Skip();
		parent->SetOption(agi::make_unique<agi::OptionValueString>(name, from_wx(evt.GetString())));
	}
};

class IntUpdater {
	std::string name;
	Preferences *parent;
public:
	IntUpdater(std::string const& n, Preferences *p) : name(n), parent(p) { }
	void operator()(wxSpinEvent& evt) {
		evt.Skip();
		parent->SetOption(agi::make_unique<agi::OptionValueInt>(name, evt.GetInt()));
	}
};

class IntCBUpdater {
	std::string name;
	Preferences *parent;
public:
	IntCBUpdater(std::string const& n, Preferences *p) : name(n), parent(p) { }
	void operator()(wxCommandEvent& evt) {
		evt.Skip();
		parent->SetOption(agi::make_unique<agi::OptionValueInt>(name, evt.GetInt()));
	}
};

class DoubleUpdater {
	std::string name;
	Preferences *parent;
public:
	DoubleUpdater(std::string const& n, Preferences *p) : name(n), parent(p) { }
	void operator()(wxSpinDoubleEvent& evt) {
		evt.Skip();
		parent->SetOption(agi::make_unique<agi::OptionValueDouble>(name, evt.GetValue()));
	}
};

class BoolUpdater {
	std::string name;
	Preferences *parent;
public:
	BoolUpdater(std::string const& n, Preferences *p) : name(n), parent(p) { }
	void operator()(wxCommandEvent& evt) {
		evt.Skip();
		parent->SetOption(agi::make_unique<agi::OptionValueBool>(name, !!evt.GetInt()));
	}
};

class ColourUpdater {
	std::string name;
	Preferences *parent;
public:
	ColourUpdater(std::string const& n, Preferences *p) : name(n), parent(p) { }
	void operator()(ValueEvent<agi::Color>& evt) {
		evt.Skip();
		parent->SetOption(agi::make_unique<agi::OptionValueColor>(name, evt.Get()));
	}
};

static void browse_button(wxTextCtrl *ctrl) {
	wxDirDialog dlg(nullptr, _("Please choose the folder:"), config::path->Decode(from_wx(ctrl->GetValue())).wstring());
	if (dlg.ShowModal() == wxID_OK) {
		wxString dir = dlg.GetPath();
		if (!dir.empty())
			ctrl->SetValue(dir);
	}
}

static void font_button(Preferences *parent, wxTextCtrl *name, wxSpinCtrl *size) {
	wxFont font;
	font.SetFaceName(name->GetValue());
	font.SetPointSize(size->GetValue());
	font = wxGetFontFromUser(parent, font);
	if (font.IsOk()) {
		name->SetValue(font.GetFaceName());
		size->SetValue(font.GetPointSize());
		// wxGTK doesn't generate wxEVT_SPINCTRL from SetValue
		wxSpinEvent evt(wxEVT_SPINCTRL);
		evt.SetInt(font.GetPointSize());
		size->ProcessWindowEvent(evt);
	}
}

OptionPage::OptionPage(wxTreebook *book, Preferences *parent, wxString name, int style)
: wxScrolled<wxPanel>(book, -1, wxDefaultPosition, wxDefaultSize, wxVSCROLL)
, sizer(new wxBoxSizer(wxVERTICAL))
, parent(parent)
{
	if (style & PAGE_SUB)
		book->AddSubPage(this, name, true);
	else
		book->AddPage(this, name, true);

	if (style & PAGE_SCROLL)
		SetScrollbars(0, 20, 0, 50);
	else
		SetScrollbars(0, 0, 0, 0);
	DisableKeyboardScrolling();
}

template<class T>
void OptionPage::Add(wxSizer *sizer, wxString const& label, T *control) {
	sizer->Add(new wxStaticText(this, -1, label), 1, wxALIGN_CENTRE_VERTICAL);
	sizer->Add(control, wxSizerFlags().Expand());
}

void OptionPage::CellSkip(wxFlexGridSizer *flex) {
	flex->AddStretchSpacer();
}

wxControl *OptionPage::OptionAddBool(wxFlexGridSizer *flex, const wxString &name, const char *opt_name) {
	parent->AddChangeableOption(opt_name);
	const auto opt = OPT_GET(opt_name);
	auto cb = new wxCheckBox(this, -1, name);
	flex->Add(cb, 1, wxEXPAND, 0);
	cb->SetValue(opt->GetBool());
	cb->Bind(wxEVT_CHECKBOX, BoolUpdater(opt_name, parent));
	return cb;
}

wxControl *OptionPage::OptionAddInt(wxFlexGridSizer *flex, const wxString &name, const char *opt_name, double min, double max) {
	parent->AddChangeableOption(opt_name);
	const auto opt = OPT_GET(opt_name);
	auto sc = new wxSpinCtrl(this, -1, std::to_wstring((int)opt->GetInt()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, min, max, opt->GetInt());
	sc->Bind(wxEVT_SPINCTRL, IntUpdater(opt_name, parent));
	Add(flex, name, sc);
	return sc;
}

wxControl *OptionPage::OptionAddDouble(wxFlexGridSizer *flex, const wxString &name, const char *opt_name, double min, double max, double inc) {
	parent->AddChangeableOption(opt_name);
	const auto opt = OPT_GET(opt_name);
	auto scd = new wxSpinCtrlDouble(this, -1, std::to_wstring(opt->GetDouble()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, min, max, opt->GetDouble(), inc);
	scd->Bind(wxEVT_SPINCTRLDOUBLE, DoubleUpdater(opt_name, parent));
	Add(flex, name, scd);
	return scd;
}

wxControl *OptionPage::OptionAddString(wxFlexGridSizer *flex, const wxString &name, const char *opt_name) {
	parent->AddChangeableOption(opt_name);
	const auto opt = OPT_GET(opt_name);
	auto text = new wxTextCtrl(this, -1 , to_wx(opt->GetString()));
	text->Bind(wxEVT_TEXT, StringUpdater(opt_name, parent));
	Add(flex, name, text);
	return text;
}

wxControl *OptionPage::OptionAddColor(wxFlexGridSizer *flex, const wxString &name, const char *opt_name) {
	parent->AddChangeableOption(opt_name);
	const auto opt = OPT_GET(opt_name);
	auto cb = new ColourButton(this, wxSize(40,10), false, opt->GetColor());
	cb->Bind(EVT_COLOR, ColourUpdater(opt_name, parent));
	Add(flex, name, cb);
	return cb;
}

void OptionPage::OptionChoiceInt(wxFlexGridSizer *flex, const wxString &name, const wxArrayString &choices, const char *opt_name) {
	parent->AddChangeableOption(opt_name);
	const auto opt = OPT_GET(opt_name);

	auto cb = new wxComboBox(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, choices, wxCB_READONLY | wxCB_DROPDOWN);
	Add(flex, name, cb);

	int val = opt->GetInt();
	cb->Select(val < (int)choices.size() ? val : 0);
	cb->Bind(wxEVT_COMBOBOX, IntCBUpdater(opt_name, parent));
}

void OptionPage::OptionChoiceString(wxFlexGridSizer *flex, const wxString &name, const wxArrayString &choices, const char *opt_name) {
	parent->AddChangeableOption(opt_name);
	const auto opt = OPT_GET(opt_name);

	auto cb = new wxComboBox(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, choices, wxCB_READONLY | wxCB_DROPDOWN);
	Add(flex, name, cb);

	wxString val(to_wx(opt->GetString()));
	if (cb->FindString(val) != wxNOT_FOUND)
		cb->SetStringSelection(val);
	else if (!choices.empty())
		cb->SetSelection(0);
	cb->Bind(wxEVT_COMBOBOX, StringUpdater(opt_name, parent));
}

wxFlexGridSizer* OptionPage::PageSizer(wxString name) {
	auto tmp_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, name);
	sizer->Add(tmp_sizer, 0,wxEXPAND, 5);
	auto flex = new wxFlexGridSizer(2,5,5);
	flex->AddGrowableCol(0,1);
	tmp_sizer->Add(flex, 1, wxEXPAND, 5);
	sizer->AddSpacer(8);
	return flex;
}

void OptionPage::OptionBrowse(wxFlexGridSizer *flex, const wxString &name, const char *opt_name, wxControl *enabler, bool do_enable) {
	parent->AddChangeableOption(opt_name);
	const auto opt = OPT_GET(opt_name);

	if (opt->GetType() != agi::OptionType::String)
		throw agi::InternalError("Option must be agi::OptionType::String for BrowseButton.");

	auto text = new wxTextCtrl(this, -1 , to_wx(opt->GetString()));
	text->SetMinSize(wxSize(160, -1));
	text->Bind(wxEVT_TEXT, StringUpdater(opt_name, parent));

	auto browse = new wxButton(this, -1, _("Browse..."));
	browse->Bind(wxEVT_BUTTON, std::bind(browse_button, text));

	auto button_sizer = new wxBoxSizer(wxHORIZONTAL);
	button_sizer->Add(text, wxSizerFlags(1).Expand());
	button_sizer->Add(browse, wxSizerFlags().Expand());

	Add(flex, name, button_sizer);

	if (enabler) {
		if (do_enable) {
			EnableIfChecked(enabler, text);
			EnableIfChecked(enabler, browse);
		}
		else {
			DisableIfChecked(enabler, text);
			DisableIfChecked(enabler, browse);
		}
	}
}

void OptionPage::OptionFont(wxSizer *sizer, std::string opt_prefix) {
	const auto face_opt = OPT_GET(opt_prefix + "Font Face");
	const auto size_opt = OPT_GET(opt_prefix + "Font Size");

	parent->AddChangeableOption(face_opt->GetName());
	parent->AddChangeableOption(size_opt->GetName());

	auto font_name = new wxTextCtrl(this, -1, to_wx(face_opt->GetString()));
	font_name->SetMinSize(wxSize(160, -1));
	font_name->Bind(wxEVT_TEXT, StringUpdater(face_opt->GetName().c_str(), parent));

	auto font_size = new wxSpinCtrl(this, -1, std::to_wstring((int)size_opt->GetInt()), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 3, 42, size_opt->GetInt());
	font_size->Bind(wxEVT_SPINCTRL, IntUpdater(size_opt->GetName().c_str(), parent));

	auto pick_btn = new wxButton(this, -1, _("Choose..."));
	pick_btn->Bind(wxEVT_BUTTON, std::bind(font_button, parent, font_name, font_size));

	auto button_sizer = new wxBoxSizer(wxHORIZONTAL);
	button_sizer->Add(font_name, wxSizerFlags(1).Expand());
	button_sizer->Add(pick_btn, wxSizerFlags().Expand());

	Add(sizer, _("Font Face"), button_sizer);
	Add(sizer, _("Font Size"), font_size);
}

void OptionPage::EnableIfChecked(wxControl *cbx, wxControl *ctrl) {
	auto cb = dynamic_cast<wxCheckBox*>(cbx);
	if (!cb) return;

	ctrl->Enable(cb->IsChecked());
	cb->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent& evt) { ctrl->Enable(!!evt.GetInt()); evt.Skip(); });
}

void OptionPage::DisableIfChecked(wxControl *cbx, wxControl *ctrl) {
	auto cb = dynamic_cast<wxCheckBox*>(cbx);
	if (!cb) return;

	ctrl->Enable(!cb->IsChecked());
	cb->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent& evt) { ctrl->Enable(!evt.GetInt()); evt.Skip(); });
}
