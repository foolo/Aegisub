// Copyright (c) 2014, Thomas Goyne <plorkyeran@aegisub.org>
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
//
// Aegisub Project http://www.aegisub.org/

#include <libaegisub/option_value.h>

namespace agi {
	std::string OptionValue::TypeToString(OptionType type) const {
		switch (type) {
			case OptionType::String:     return "String";
			case OptionType::Int:        return "Integer";
			case OptionType::Double:     return "Double";
			case OptionType::Color:      return "Color";
			case OptionType::Bool:       return "Bool";
			case OptionType::ListString: return "List of Strings";
			case OptionType::ListInt:    return "List of Integers";
			case OptionType::ListDouble: return "List of Doubles";
			case OptionType::ListColor:  return "List of Colors";
			case OptionType::ListBool:   return "List of Bools";
		}
		throw agi::InternalError("Invalid option type");
	}

	InternalError OptionValue::TypeError(OptionType type) const {
		return InternalError("Invalid type for option " + name + ": expected " +
			TypeToString(type) + ", got " + TypeToString(GetType()));
	}

void OptionValueString::Set(const OptionValue *nv) { SetValue(nv->GetString()); }
void OptionValueListString::Set(const OptionValue *nv) { SetValue(nv->GetListString()); }

void OptionValueInt::Set(const OptionValue *nv) { SetValue(nv->GetInt()); }
void OptionValueListInt::Set(const OptionValue *nv) { SetValue(nv->GetListInt()); }

void OptionValueDouble::Set(const OptionValue *nv) { SetValue(nv->GetDouble()); }
void OptionValueListDouble::Set(const OptionValue *nv) { SetValue(nv->GetListDouble()); }

void OptionValueColor::Set(const OptionValue *nv) { SetValue(nv->GetColor()); }
void OptionValueListColor::Set(const OptionValue *nv) { SetValue(nv->GetListColor()); }

void OptionValueBool::Set(const OptionValue *nv) { SetValue(nv->GetBool()); }
void OptionValueListBool::Set(const OptionValue *nv) { SetValue(nv->GetListBool()); }

std::string OptionValue::nullValueString;
int64_t OptionValue::nullValueInt64 = 0;
double OptionValue::nullValueDouble = 0.0;
Color OptionValue::nullValueColor;
bool OptionValue::nullValueBool = false;

std::vector<std::string> OptionValue::nullValueStringVector;
std::vector<int64_t> OptionValue::nullValueInt64Vector;
std::vector<double> OptionValue::nullValueDoubleVector;
std::vector<Color> OptionValue::nullValueColorVector;
std::vector<bool> OptionValue::nullValueBoolVector;

}
