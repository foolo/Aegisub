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

	static std::string nullValueString;
	static int64_t nullValueInt64;
	static double nullValueDouble;
	static Color nullValueColor;
	static bool nullValueBool;

	static std::vector<std::string> nullValueStringVector;
	static std::vector<int64_t> nullValueInt64Vector;
	static std::vector<double> nullValueDoubleVector;
	static std::vector<Color> nullValueColorVector;
	static std::vector<bool> nullValueBoolVector;

protected:
	void NotifyChanged() { ValueChanged(*this); }

	OptionValue(std::string name) BOOST_NOEXCEPT : name(std::move(name)) { }

public:
	virtual ~OptionValue() = default;

	std::string const& GetName() const { return name; }
	virtual OptionType GetType() const = 0;
	virtual bool IsDefault() const = 0;
	virtual void Reset() = 0;

	virtual std::string const& GetString() const { return nullValueString; };
	virtual int64_t const& GetInt() const { return nullValueInt64; }
	virtual double const& GetDouble() const { return nullValueDouble; };
	virtual Color const& GetColor() const { return nullValueColor; };
	virtual bool const& GetBool() const { return nullValueBool; };

	void SetString(const std::string);
	void SetInt(const int64_t);
	void SetDouble(const double);
	void SetColor(const Color);
	void SetBool(const bool);

	virtual std::vector<std::string> const& GetListString() const { return nullValueStringVector; };
	virtual std::vector<int64_t> const& GetListInt() const { return nullValueInt64Vector; };
	virtual std::vector<double> const& GetListDouble() const { return nullValueDoubleVector; };
	virtual std::vector<Color> const& GetListColor() const { return nullValueColorVector; };
	virtual std::vector<bool> const& GetListBool() const { return nullValueBoolVector; };

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
		std::string const& GetString() const { return value; };
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
		int64_t const& GetInt() const { return value; }
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
		double const& GetDouble() const { return value; };
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
		Color const& GetColor() const { return value; };
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
		bool const& GetBool() const { return value; };
		void SetValue(bool new_val) { value = std::move(new_val); NotifyChanged(); }
		OptionType GetType() const { return OptionType::Bool; }
		void Reset() { value = value_default; NotifyChanged(); }
		bool IsDefault() const { return value == value_default; }
		void Set(const OptionValue *nv);
	};

class OptionValueListString final : public OptionValue {
		std::vector<std::string> array;
		std::vector<std::string> array_default;
		std::string name;
	public:
		typedef std::vector<std::string> value_type;
		OptionValueListString(std::string name, std::vector<std::string> const& value = std::vector<std::string>())
		: OptionValue(std::move(name))
		, array(value), array_default(value) { }
		std::vector<std::string> const& GetListString() const { return array; };
		void SetValue(std::vector<std::string> val) { array = std::move(val); NotifyChanged(); }
		OptionType GetType() const { return OptionType::ListString; }
		void Reset() { array = array_default; NotifyChanged(); }
		bool IsDefault() const { return array == array_default; }
		void Set(const OptionValue *nv);
	};

class OptionValueListInt final : public OptionValue {
		std::vector<int64_t> array;
		std::vector<int64_t> array_default;
		std::string name;
	public:
		typedef std::vector<int64_t> value_type;
		OptionValueListInt(std::string name, std::vector<int64_t> const& value = std::vector<int64_t>())
		: OptionValue(std::move(name))
		, array(value), array_default(value) { }
		std::vector<int64_t> const& GetListInt() const { return array; };
		void SetValue(std::vector<int64_t> val) { array = std::move(val); NotifyChanged(); }
		OptionType GetType() const { return OptionType::ListInt; }
		void Reset() { array = array_default; NotifyChanged(); }
		bool IsDefault() const { return array == array_default; }
		void Set(const OptionValue *nv);
	};

class OptionValueListDouble final : public OptionValue {
		std::vector<double> array;
		std::vector<double> array_default;
		std::string name;
	public:
		typedef std::vector<double> value_type;
		OptionValueListDouble(std::string name, std::vector<double> const& value = std::vector<double>())
		: OptionValue(std::move(name))
		, array(value), array_default(value) { }
		std::vector<double> const& GetListDouble() const { return array; };
		void SetValue(std::vector<double> val) { array = std::move(val); NotifyChanged(); }
		OptionType GetType() const { return OptionType::ListDouble; }
		void Reset() { array = array_default; NotifyChanged(); }
		bool IsDefault() const { return array == array_default; }
		void Set(const OptionValue *nv);
	};

class OptionValueListColor final : public OptionValue {
		std::vector<Color> array;
		std::vector<Color> array_default;
		std::string name;
	public:
		typedef std::vector<Color> value_type;
		OptionValueListColor(std::string name, std::vector<Color> const& value = std::vector<Color>())
		: OptionValue(std::move(name))
		, array(value), array_default(value) { }
		std::vector<Color> const& GetListColor() const { return array; };
		void SetValue(std::vector<Color> val) { array = std::move(val); NotifyChanged(); }
		OptionType GetType() const { return OptionType::ListColor; }
		void Reset() { array = array_default; NotifyChanged(); }
		bool IsDefault() const { return array == array_default; }
		void Set(const OptionValue *nv);
	};

class OptionValueListBool final : public OptionValue {
		std::vector<bool> array;
		std::vector<bool> array_default;
		std::string name;
	public:
		typedef std::vector<bool> value_type;
		OptionValueListBool(std::string name, std::vector<bool> const& value = std::vector<bool>())
		: OptionValue(std::move(name))
		, array(value), array_default(value) { }
		std::vector<bool> const& GetListBool() const { return array; };
		void SetValue(std::vector<bool> val) { array = std::move(val); NotifyChanged(); }
		OptionType GetType() const { return OptionType::ListBool; }
		void Reset() { array = array_default; NotifyChanged(); }
		bool IsDefault() const { return array == array_default; }
		void Set(const OptionValue *nv);
	};

inline void OptionValue::SetString(std::string v) { As<OptionValueString>(OptionType::String)->SetValue(std::move(v)); }

inline void OptionValue::SetInt(int64_t v) { As<OptionValueInt>(OptionType::Int)->SetValue(std::move(v)); }
inline void OptionValue::SetDouble(double v) { As<OptionValueDouble>(OptionType::Double)->SetValue(std::move(v)); }

inline void OptionValue::SetColor(Color v) { As<OptionValueColor>(OptionType::Color)->SetValue(std::move(v)); }

inline void OptionValue::SetBool(bool v) { As<OptionValueBool>(OptionType::Bool)->SetValue(std::move(v)); }

inline void OptionValue::SetListString(std::vector<std::string> v) { As<OptionValueListString>(OptionType::ListString)->SetValue(std::move(v)); }

inline void OptionValue::SetListInt(std::vector<int64_t> v) { As<OptionValueListInt>(OptionType::ListInt)->SetValue(std::move(v)); }

inline void OptionValue::SetListDouble(std::vector<double> v) { As<OptionValueListDouble>(OptionType::ListDouble)->SetValue(std::move(v)); }

inline void OptionValue::SetListColor(std::vector<Color> v) { As<OptionValueListColor>(OptionType::ListColor)->SetValue(std::move(v)); }

inline void OptionValue::SetListBool(std::vector<bool> v) { As<OptionValueListBool>(OptionType::ListBool)->SetValue(std::move(v)); }

} // namespace agi
