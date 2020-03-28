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

/// @file option_value.h
/// @brief Container for holding an actual option value.
/// @ingroup libaegisub

#include <cstdint>
#include <vector>

#include <libaegisub/color.h>
#include <libaegisub/exception.h>
#include <libaegisub/signal.h>

// X11 is awesome and defines Bool to int
#undef Bool

namespace agi {
/// Option type
/// No bitsets here.
enum class OptionType {
	String     = 0,	///< String
	Int        = 1,	///< Integer
	Double     = 2,	///< Double
	Color      = 3,	///< Color
	Bool       = 4,	///< Bool
	ListString = 100,	///< List of Strings
	ListInt    = 101,	///< List of Integers
	ListDouble = 102,	///< List of Doubles
	ListColor  = 103,	///< List of Colors
	ListBool   = 104	///< List of Bools
};

/// @class OptionValue
/// Holds an actual option.
class OptionValue {
	agi::signal::Signal<OptionValue const&> ValueChanged;
	std::string name;

	std::string TypeToString(OptionType type) const;
	InternalError TypeError(OptionType type) const;

	template<typename T>
	T *As(OptionType type) {
		if (GetType() == type)
			return static_cast<T *>(this);
		throw TypeError(type);
	}

	template<typename T>
	const T *As(OptionType type) const {
		if (GetType() == type)
			return static_cast<const T *>(this);
		throw TypeError(type);
	}

protected:
	void NotifyChanged() { ValueChanged(*this); }

	OptionValue(std::string name) BOOST_NOEXCEPT : name(std::move(name)) { }

public:
	virtual ~OptionValue() = default;

	std::string const& GetName() const { return name; }
	virtual OptionType GetType() const = 0;
	virtual bool IsDefault() const = 0;
	virtual void Reset() = 0;

	std::string const& GetString() const;
	int64_t const& GetInt() const;
	double const& GetDouble() const;
	Color const& GetColor() const;
	bool const& GetBool() const;

	void SetString(const std::string);
	void SetInt(const int64_t);
	void SetDouble(const double);
	void SetColor(const Color);
	void SetBool(const bool);

	std::vector<std::string> const& GetListString() const;
	std::vector<int64_t> const& GetListInt() const;
	std::vector<double> const& GetListDouble() const;
	std::vector<Color> const& GetListColor() const;
	std::vector<bool> const& GetListBool() const;

	void SetListString(std::vector<std::string>);
	void SetListInt(std::vector<int64_t>);
	void SetListDouble(std::vector<double>);
	void SetListColor(std::vector<Color>);
	void SetListBool(std::vector<bool>);

	virtual void Set(const OptionValue *new_value)=0;

	DEFINE_SIGNAL_ADDERS(ValueChanged, Subscribe)
};

class OptionValueString final : public OptionValue {
		std::string value;
		std::string value_default;
	public:
		typedef std::string value_type;
		OptionValueString(std::string member_name, std::string member_value)
		: OptionValue(std::move(member_name))
		, value(member_value), value_default(member_value) { }
		std::string const& GetValue() const { return value; }
		void SetValue(std::string new_val) { value = std::move(new_val); NotifyChanged(); }
		OptionType GetType() const { return OptionType::String; }
		void Reset() { value = value_default; NotifyChanged(); }
		bool IsDefault() const { return value == value_default; }
		void Set(const OptionValue *nv);
	};

class OptionValueInt final : public OptionValue {
		int64_t value;
		int64_t value_default;
	public:
		typedef int64_t value_type;
		OptionValueInt(std::string member_name, int64_t member_value)
		: OptionValue(std::move(member_name))
		, value(member_value), value_default(member_value) { }
		int64_t const& GetValue() const { return value; }
		void SetValue(int64_t new_val) { value = std::move(new_val); NotifyChanged(); }
		OptionType GetType() const { return OptionType::Int; }
		void Reset() { value = value_default; NotifyChanged(); }
		bool IsDefault() const { return value == value_default; }
		void Set(const OptionValue *nv);
	};

class OptionValueDouble final : public OptionValue {
		double value;
		double value_default;
	public:
		typedef double value_type;
		OptionValueDouble(std::string member_name, double member_value)
		: OptionValue(std::move(member_name))
		, value(member_value), value_default(member_value) { }
		double const& GetValue() const { return value; }
		void SetValue(double new_val) { value = std::move(new_val); NotifyChanged(); }
		OptionType GetType() const { return OptionType::Double; }
		void Reset() { value = value_default; NotifyChanged(); }
		bool IsDefault() const { return value == value_default; }
		void Set(const OptionValue *nv);
	};

class OptionValueColor final : public OptionValue {
		Color value;
		Color value_default;
	public:
		typedef Color value_type;
		OptionValueColor(std::string member_name, Color member_value)
		: OptionValue(std::move(member_name))
		, value(member_value), value_default(member_value) { }
		Color const& GetValue() const { return value; }
		void SetValue(Color new_val) { value = std::move(new_val); NotifyChanged(); }
		OptionType GetType() const { return OptionType::Color; }
		void Reset() { value = value_default; NotifyChanged(); }
		bool IsDefault() const { return value == value_default; }
		void Set(const OptionValue *nv);
	};

class OptionValueBool final : public OptionValue {
		bool value;
		bool value_default;
	public:
		typedef bool value_type;
		OptionValueBool(std::string member_name, bool member_value)
		: OptionValue(std::move(member_name))
		, value(member_value), value_default(member_value) { }
		bool const& GetValue() const { return value; }
		void SetValue(bool new_val) { value = std::move(new_val); NotifyChanged(); }
		OptionType GetType() const { return OptionType::Bool; }
		void Reset() { value = value_default; NotifyChanged(); }
		bool IsDefault() const { return value == value_default; }
		void Set(const OptionValue *nv);
	};

#define CONFIG_OPTIONVALUE_LIST(type_name, type)                                          \
	class OptionValueList##type_name final : public OptionValue {                         \
		std::vector<type> array;                                                          \
		std::vector<type> array_default;                                                  \
		std::string name;                                                                 \
	public:                                                                               \
		typedef std::vector<type> value_type;                                             \
		OptionValueList##type_name(std::string name, std::vector<type> const& value = std::vector<type>()) \
		: OptionValue(std::move(name))                                                    \
		, array(value), array_default(value) { }                                          \
		std::vector<type> const& GetValue() const { return array; }                       \
		void SetValue(std::vector<type> val) { array = std::move(val); NotifyChanged(); } \
		OptionType GetType() const { return OptionType::List##type_name; }                \
		void Reset() { array = array_default; NotifyChanged(); }                          \
		bool IsDefault() const { return array == array_default; }                         \
		void Set(const OptionValue *nv);                                                  \
	};

CONFIG_OPTIONVALUE_LIST(String, std::string)
CONFIG_OPTIONVALUE_LIST(Int, int64_t)
CONFIG_OPTIONVALUE_LIST(Double, double)
CONFIG_OPTIONVALUE_LIST(Color, Color)
CONFIG_OPTIONVALUE_LIST(Bool, bool)

inline std::string const& OptionValue::GetString() const { return As<OptionValueString>(OptionType::String)->GetValue(); }
inline void OptionValue::SetString(std::string v) { As<OptionValueString>(OptionType::String)->SetValue(std::move(v)); }

inline int64_t const& OptionValue::GetInt() const { return As<OptionValueInt>(OptionType::Int)->GetValue(); }
inline void OptionValue::SetInt(int64_t v) { As<OptionValueInt>(OptionType::Int)->SetValue(std::move(v)); }

inline double const& OptionValue::GetDouble() const { return As<OptionValueDouble>(OptionType::Double)->GetValue(); }
inline void OptionValue::SetDouble(double v) { As<OptionValueDouble>(OptionType::Double)->SetValue(std::move(v)); }

inline Color const& OptionValue::GetColor() const { return As<OptionValueColor>(OptionType::Color)->GetValue(); }
inline void OptionValue::SetColor(Color v) { As<OptionValueColor>(OptionType::Color)->SetValue(std::move(v)); }

inline bool const& OptionValue::GetBool() const { return As<OptionValueBool>(OptionType::Bool)->GetValue(); }
inline void OptionValue::SetBool(bool v) { As<OptionValueBool>(OptionType::Bool)->SetValue(std::move(v)); }

inline std::vector<std::string> const& OptionValue::GetListString() const { return As<OptionValueListString>(OptionType::ListString)->GetValue(); }
inline void OptionValue::SetListString(std::vector<std::string> v) { As<OptionValueListString>(OptionType::ListString)->SetValue(std::move(v)); }

inline std::vector<int64_t> const& OptionValue::GetListInt() const { return As<OptionValueListInt>(OptionType::ListInt)->GetValue(); }
inline void OptionValue::SetListInt(std::vector<int64_t> v) { As<OptionValueListInt>(OptionType::ListInt)->SetValue(std::move(v)); }

inline std::vector<double> const& OptionValue::GetListDouble() const { return As<OptionValueListDouble>(OptionType::ListDouble)->GetValue(); }
inline void OptionValue::SetListDouble(std::vector<double> v) { As<OptionValueListDouble>(OptionType::ListDouble)->SetValue(std::move(v)); }

inline std::vector<Color> const& OptionValue::GetListColor() const { return As<OptionValueListColor>(OptionType::ListColor)->GetValue(); }
inline void OptionValue::SetListColor(std::vector<Color> v) { As<OptionValueListColor>(OptionType::ListColor)->SetValue(std::move(v)); }

inline std::vector<bool> const& OptionValue::GetListBool() const { return As<OptionValueListBool>(OptionType::ListBool)->GetValue(); }
inline void OptionValue::SetListBool(std::vector<bool> v) { As<OptionValueListBool>(OptionType::ListBool)->SetValue(std::move(v)); }

#undef CONFIG_OPTIONVALUE_LIST
} // namespace agi
