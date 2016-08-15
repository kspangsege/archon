/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_DOM_IMPL_STYLE_HPP
#define ARCHON_DOM_IMPL_STYLE_HPP

#include <stdexcept>
#include <locale>
#include <string>
#include <iomanip>
#include <sstream>

#include <archon/core/unique_ptr.hpp>
#include <archon/core/enum.hpp>
#include <archon/core/memory.hpp>
#include <archon/math/vector.hpp>
#include <archon/util/packed_trgb.hpp>
#include <archon/util/named_colors.hpp>
#include <archon/dom/css.hpp>
#include <archon/dom/impl/ls.hpp>


namespace archon
{
  namespace DomImpl
  {
    class StyleApplyee;
    class ElemStyleDecl;
    class StyledElement;
    class StyledElemType;
    class ElemStyleDeclManager;
    class StyledDocument;
    class StyledImplementation;




    enum CssLevel
    {
      css21, ///< Level 2 Revision 1
      css3   ///< Level 3
    };




    enum StylePriority { style_priority_Normal, style_priority_Important };




    enum ValueType
    {
      value_Unspecified,
      value_Inherit,
      value_Auto,
      value_Transparent, // Deprecated in CSS3
      value_RGB_Number, value_RGB_Percent,
      value_HSL_Number, value_HSL_Percent,
      _value_End     ///< This one is just a marker
    };




    enum LengthUnit
    {
      lengthUnit_None, lengthUnit_Percent, lengthUnit_EM, lengthUnit_EX, lengthUnit_PX,
      lengthUnit_CM, lengthUnit_MM, lengthUnit_IN, lengthUnit_PT, lengthUnit_PC,
      _lengthUnit_End     ///< This one is just a marker
    };




    enum NamedBorderWidth
    {
      borderWidth_Thin, borderWidth_Medium, borderWidth_Thick,
      _borderWidth_End     ///< This one is just a marker
    };




    enum BorderStyle
    {
      borderStyle_None, borderStyle_Hidden, borderStyle_Dotted, borderStyle_Dashed,
      borderStyle_Solid, borderStyle_Double, borderStyle_Groove, borderStyle_Ridge,
      borderStyle_Inset, borderStyle_Outset
    };




    enum FontStyle
    {
      fontStyle_Normal, fontStyle_Italic, fontStyle_Oblique
    };




    enum FontVariant
    {
      fontVariant_Normal, fontVariant_SmallCaps
    };




    enum FontWeight
    {
      fontWeight_100, fontWeight_200, fontWeight_300, fontWeight_400, fontWeight_500,
      fontWeight_600, fontWeight_700, fontWeight_800, fontWeight_900
    };




    enum SpecialFontWeight
    {
      specialFontWeight_Normal, specialFontWeight_Bold,
      specialFontWeight_Bolder, specialFontWeight_Lighter,
      _specialFontWeight_End     ///< This one is just a marker
    };




    enum NamedFontSize
    {
      fontSize_XXSmall, fontSize_XSmall, fontSize_Small, fontSize_Medium,
      fontSize_Large, fontSize_XLarge, fontSize_XXLarge,
      fontSize_Larger, fontSize_Smaller,
      _fontSize_End     ///< This one is just a marker
    };




    enum SystemColor
    {
      sysColor_ActiveBorder, sysColor_ActiveCaption, sysColor_AppWorkspace,
      sysColor_Background, sysColor_ButtonFace, sysColor_ButtonHighlight,
      sysColor_ButtonShadow, sysColor_ButtonText, sysColor_CaptionText,
      sysColor_GrayText, sysColor_Highlight, sysColor_HighlightText,
      sysColor_InactiveBorder, sysColor_InactiveCaption, sysColor_InactiveCaptionText,
      sysColor_InfoBackground, sysColor_InfoText, sysColor_Menu, sysColor_MenuText,
      sysColor_Scrollbar, sysColor_ThreeDDarkShadow, sysColor_ThreeDFace,
      sysColor_ThreeDHighlight, sysColor_ThreeDLightShadow, sysColor_ThreeDShadow,
      sysColor_Window, sysColor_WindowFrame, sysColor_WindowText,
      _sysColor_End     ///< This one is just a marker
    };



    enum ClearValue
    {
      clear_None, clear_Left, clear_Right, clear_Both
    };



    enum DisplayValue
    {
      display_Inline, display_Block, display_ListItem, display_InlineBlock,
      display_Table, display_InlineTable, display_TableRowGroup, display_TableHeaderGroup,
      display_TableFooterGroup, display_TableRow, display_TableColumnGroup,
      display_TableColumn, display_TableCell, display_TableCaption, display_None
    };



    enum FloatValue
    {
      float_Left, float_Right, float_None
    };



    enum OverflowValue
    {
      overflow_Visible, overflow_Hidden, overflow_Scroll
    };



    enum PositionValue
    {
      position_Static, position_Relative, position_Absolute, position_Fixed
    };




    // Properties are marked dirty and reported to the rendering
    // application in groups. The 'font' group must always be
    // applied first, such that properties in the other groups can
    // refer reliably to the current font size, as well as to the
    // current height of 'x'. The 'font' group consists precisely of
    // 'font-style', 'font-variant', 'font-weight', 'font-size', and
    // 'font-family'.
    enum StyleGroup
    {
      style_group_Font, style_group_Text, style_group_Background, style_group_Border,
      style_group_Margin, style_group_Padding, style_group_Size, style_group_Structure
    };




    struct ComputedStyle
    {
      typedef double Length;

      struct AugmentedLength
      {
        enum State { state_Auto, state_Abs, state_Rel } state;
        Length value;
        AugmentedLength() {}
        AugmentedLength(State s, Length v): state(s), value(v) {}
        bool operator==(AugmentedLength const &l) const
        {
          switch (state) {
          default:         return l.state == state_Auto;
          case state_Abs:  return l.state == state_Abs && value == l.value;
          case state_Rel:  return l.state == state_Rel && value == l.value;
          }
        }
        bool operator!=(AugmentedLength const &l) const { return !operator==(l); }
      };

      typedef Util::PackedTRGB Color;


      // The 'font' group must always be applied first, such that
      // properties in the other groups can be computed reliably with
      // respect to the current font size, as well as to the current
      // height of 'x'. The 'font' group consists precisely of
      // 'font-style', 'font-variant', 'font-weight', 'font-size', and
      // 'font-family'.
      struct Font
      {
        FontStyle   style;
        FontVariant variant;
        FontWeight  weight;
        Length      size;
        void init()
        {
          style   = fontStyle_Normal;
          variant = fontVariant_Normal;
          weight  = fontWeight_400;
          size    = 0;
        }
      } font;

      // This group is intended to contain most (if not all) of the
      // default inherited properties that relate to text and is not
      // on the 'font' group.
      struct Text
      {
        Color color;
        AugmentedLength line_height;
        void init()
        {
          color = Util::Color::white;
          line_height.state = AugmentedLength::state_Auto; // 'normal'
        }
      } text;

      struct Background
      {
        Color color;
        void init() { color = Util::Color::transparent; }
      } background;

      struct Border
      {
        struct Side
        {
          /**
           * If the border style is 'none' or 'hidden', then the
           * computed value is indeed zero. This field, however, is
           * not guaranteed to be set to zero in that case. The right
           * value is always returned by reading it through the
           * appropriate specifyer class for this property.
           */
          Length width;

          BorderStyle style;

          /**
           * The true computed value for the border color is the
           * current text color if a value has not yet been
           * specified. The right value is always returned by reading
           * it through the appropriate specifyer class for this
           * property.
           */
          Color color;
          bool color_specified;

          void init() { width = 0; style = borderStyle_None; color_specified = false; }
        } top, right, bottom, left;
        void init() { top.init(); right.init(); bottom.init(); left.init(); }
        void set_width(Length w) { top.width = right.width = bottom.width = left.width = w; }
      } border;

      struct Margin
      {
        AugmentedLength top, right, bottom, left;
        void init()
        {
          top.state = right.state = bottom.state = left.state = AugmentedLength::state_Abs;
          top.value = right.value = bottom.value = left.value = 0;
        }
      } margin;

      struct Padding
      {
        AugmentedLength top, right, bottom, left;
        void init()
        {
          top.state = right.state = bottom.state = left.state = AugmentedLength::state_Abs;
          top.value = right.value = bottom.value = left.value = 0;
        }
      } padding;

      struct Size
      {
        AugmentedLength width, height;
        void init()
        {
          width.state = height.state = AugmentedLength::state_Auto;
        }
      } size;

      struct Structure
      {
        ClearValue    clear;
        DisplayValue  display;
        FloatValue    cssFloat;
        OverflowValue overflow;
        PositionValue position;
        void init()
        {
          clear    = clear_None;
          display  = display_Inline;
          cssFloat = float_None;
          overflow = overflow_Visible;
          position = position_Static;
        }
      } structure;

      void init()
      {
        font.init();
        text.init();
        background.init();
        border.init();
        margin.init();
        padding.init();
        size.init();
        structure.init();
      }

      void constraint_fixups(bool is_root)
      {
        if (structure.display == display_None) return;
        if (structure.position == position_Absolute || structure.position == position_Fixed) {
          structure.cssFloat = float_None;
          goto fix_display;
        }
        if (structure.cssFloat != float_None) goto fix_display;
        if (is_root) goto fix_display;
        return;

      fix_display:
        switch (structure.display) {

        case display_InlineTable:
          structure.display = display_Table;
          break;

        case display_Table:
        case display_Inline:
        case display_TableRowGroup:
        case display_TableColumn:
        case display_TableColumnGroup:
        case display_TableHeaderGroup:
        case display_TableFooterGroup:
        case display_TableRow:
        case display_TableCell:
        case display_TableCaption:
        case display_InlineBlock:
          structure.display = display_Block;
          break;

        default:
          break;
        }
      }
    };




    inline std::ostream &operator<<(std::ostream &out, ComputedStyle::AugmentedLength const &l)
    {
      switch (l.state) {
      case ComputedStyle::AugmentedLength::state_Auto: out << "auto"; break;
      case ComputedStyle::AugmentedLength::state_Abs:  out << "abs("<<l.value<<")"; break;
      case ComputedStyle::AugmentedLength::state_Rel:  out << "rel("<<l.value<<")"; break;
      }
      return out;
    }




    /**
     * Each distinct short or long hand CSS property has its
     * definition represented by a unique instance of this class.
     */
    struct StylePropDef
    {
      virtual dom::DOMString get(StyledElement const &) const = 0;
      virtual void set(dom::DOMString const &value, StyledElement &) const = 0;

      virtual ~StylePropDef() {}
    };




    struct StaticStyleInfo
    {
      CssLevel const css_level;

      StylePropDef const *lookup_prop_def(dom::DOMString const &name) const
      {
        PropMap::const_iterator const i = prop_map.find(name);
        if (i == prop_map.end()) return 0;
        return i->second;
      }

      StaticStyleInfo(CssLevel l): css_level(l) { add_props(); }

    private:
      void add_props();

      template<class Prop, class Group> StylePropDef &add(Prop Group::*prop);

      template<class ForceGroup, class Prop, class Group> StylePropDef &add2(Prop Group::*prop);

      template<class P> P &add(std::string name, P *prop) { add2(name, prop); return *prop; }

      StylePropDef &add2(std::string name, StylePropDef *prop)
      {
        core::UniquePtr<StylePropDef> p(prop);
        prop_map.set_at(dom::str_from_port(name), p);
        return *prop;
      }

      typedef core::DeletingMap<dom::DOMString, StylePropDef const> PropMap;
      PropMap prop_map;
    };




    struct StyleManipContext
    {
      StylePropDef const *lookup_prop_def(dom::DOMString const &name) const
      {
        return static_info.lookup_prop_def(name);
      }

      template<class Prop> dom::DOMString format_prop(Prop const &prop)
      {
        dom::DOMString str;
        prop.format_value(str, *this);
        return str;
      }

      template<class Prop> void parse_narrow_prop(dom::DOMString const &str, Prop &prop)
      {
        std::string str2;
        if (dom::str_to_narrow_port(str, str2)) {
          std::string::const_iterator i = str2.begin(), j = str2.end();
          for (;;) {
            if (i == j) {
              prop.parse_value(std::string(), *this); // Make it unspecified
              return;
            }
            if (!is_space(*i)) break;
            ++i;
          }
          for (;;) {
            std::string::const_iterator const k = j-1;
            if (!is_space(*k)) break;
            j = k;
          }
          if (prop.parse_value(std::string(i,j), *this)) return;
        }
        throw dom::DOMException(dom::SYNTAX_ERR, "Failed to parse property value");
      }

      template<class Prop> void parse_wide_prop(dom::DOMString const &str, Prop &prop)
      {
        dom::DOMString::const_iterator i = str.begin(), j = str.end();
        for (;;) {
          if (i == j) {
            prop.parse_value(dom::DOMString(), *this); // Make it unspecified
            return;
          }
          if (!is_space(*i)) break;
          ++i;
        }
        for (;;) {
          dom::DOMString::const_iterator const k = j-1;
          if (!is_space(*k)) break;
          j = k;
        }
        if (prop.parse_value(dom::DOMString(i,j), *this)) return;
        throw dom::DOMException(dom::SYNTAX_ERR, "Failed to parse property value");
      }

      static bool is_space(char c)
      {
        return
          c == ' '  || // space           (U+0020)
          c == '\f' || // form feed       (U+000C)
          c == '\n' || // newline         (U+000A)
          c == '\r' || // carriage return (U+000D)
          c == '\t';   // horizontal tab  (U+0009)
      }

      static bool is_space(dom::DOMString::value_type c)
      {
        typedef std::char_traits<dom::DOMString::value_type> traits;
        traits::int_type const i = traits::to_int_type(c);
        return
          i == 0x20 || // space           (U+0020)
          i == 0x0C || // form feed       (U+000C)
          i == 0x0A || // line feed       (U+000A)
          i == 0x0D || // carriage return (U+000D)
          i == 0x09;   // tab             (U+0009)
      }

      Util::PackedTRGB::CssLevel get_color_parser_css_level() const
      {
        switch (static_info.css_level) {
        case css21: return Util::PackedTRGB::css21;
        case css3:  break;
        }
        return Util::PackedTRGB::css3;
      }

      template<class T> void str_append_port(dom::DOMString &out, T const &v)
      {
        format_stream.str(std::string());
        format_stream << v;
        dom::str_append_port(out, format_stream.str());
      }

      StylePriority parse_priority(dom::DOMString const &prio) const
      {
        if (prio.empty())           return style_priority_Normal;
        if (prio == prio_important) return style_priority_Important;
        throw dom::DOMException(dom::SYNTAX_ERR, "Unrecognized priority");
      }

      // FIXME: A new special numeric type should be
      // introduced with the correct parsing and formatting
      // semantics. That would make the stuff below trivial.
      template<class T> bool parse_length(std::string str, T &value, std::string &unit)
      {
        T v;
        bool have_integer_part = false;
	parse_stream.clear();
        parse_stream.str(str);
        {
          // Read integer part
          int i = 0;
          parse_stream >> std::noskipws >> i;
          if (parse_stream) have_integer_part = true;
          else {
            if (parse_stream.bad()) return false;
            parse_stream.clear();
          }
          v = i;
        }
        typedef std::char_traits<char> traits;
        traits::int_type c = parse_stream.peek();
        if (c == traits::to_int_type('.')) {
          parse_stream.get(); // Get rid of the dot
          // Must have at least one digit after decimal point.
          c = parse_stream.peek();
          if (c == traits::eof()) return false;
          if (!std::isdigit(traits::to_char_type(c), parse_stream.getloc())) return false;
          // Read fractional part.
          int i;
          parse_stream >> std::noskipws >> i;
          if (!parse_stream) return false;
          v += i * std::pow(10.0, double(-parse_stream.gcount()));
        }
        else if (!have_integer_part) return false;
        std::string u;
        parse_stream >> std::noskipws >> u;
        if (!parse_stream || parse_stream.get() != traits::eof()) return false;
        value = v;
        unit = u;
        return true;
      }

      void deprecation_warning(dom::DOMString const &msg) { warning(msg); }

      void unrecognized_warning(dom::DOMString const &msg) { warning(msg); }

      StyleManipContext(StaticStyleInfo const *i):
        prio_important(dom::str_from_cloc(L"important")), static_info(*i)
      {
        format_stream.imbue(std::locale::classic());
        parse_stream.imbue(std::locale::classic());
      }

    private:
      dom::DOMString const prio_important;
      StaticStyleInfo const &static_info;
      std::ostringstream format_stream;
      std::istringstream parse_stream;

      void warning(dom::DOMString const &);
    };




    struct StyleComputeContext
    {
      double get_dpcm() const { return dpcm; }

      ComputedStyle const &get_default_style()
      {
        if (!default_style) {
          core::UniquePtr<ComputedStyle> s(new ComputedStyle);
          s->init();
          s->border.set_width(get_std_border_width(borderWidth_Medium));
          s->font.size = get_std_font_size(0);
          modif_default_style(*s);
          default_style.reset(s.release());
        }
        return *default_style;
      }

      static double get_std_border_width(NamedBorderWidth w)
      {
        switch (w) {
        case borderWidth_Thin:  return 1;
        default:                return 3;
        case borderWidth_Thick: return 5;
        }
      }

      static double get_std_font_size(int i)
      {
        return 18 * pow(get_font_size_scale_factor(), i);
      }

      static double increase_font_size(double s)
      {
        return get_font_size_scale_factor() * s;
      }

      static double decrease_font_size(double s)
      {
        return (1/get_font_size_scale_factor()) * s;
      }

/*
      ComputedStyle *acquire_comp_style()
      {
        if (unused_comp_styles.empty()) return new ComputedStyle;
        unused_copm_styles
      }
*/

      StyleComputeContext(double dpcm): dpcm(dpcm) {}

      virtual ~StyleComputeContext() {}

    protected:
      virtual void modif_default_style(ComputedStyle &) {}

    private:
      core::UniquePtr<ComputedStyle const> default_style;

      static double get_font_size_scale_factor() { return 7.0/6; }

      double dpcm;
    };




    struct StyleComputeState
    {
      ComputedStyle const &get_default_style() const { return default_style; }

      ComputedStyle &get_current_style() { return current_style; }

      double get_dpcm() const { return dpcm; }

      // FIXME: Provide an efficient allocator of ComputedStyle. Same scheme as RareStyledNodeData.
      StyleComputeState(StyleComputeContext *ctx):
        default_style(ctx->get_default_style()), current_style(default_style),
        dpcm(ctx->get_dpcm()), accum_dirty_bits(0) {}

      virtual ~StyleComputeState() {}

    protected:
      friend class StyleApplyee;

      virtual void change_font(ComputedStyle::Font const &) = 0;

      /**
       * Determine the height of 'x' given the latest font specified
       * by change_font().
       */
      virtual double determine_height_of_x() = 0;

    private:
      typedef unsigned long GroupBits;

      ComputedStyle const &default_style;
      ComputedStyle current_style;
      double dpcm;
      GroupBits accum_dirty_bits;
    };




    struct ContainingBlock
    {
      // Distance in pixels between left and right content edges.
      int width;

      // Distance in pixels between top and bottom content edges. A
      // negative value indicates that height is not yet known.
      int height;
    };




    struct StyleApplyee
    {
      bool has(StyleGroup g) const { return accum_dirty & 1ul<<g; }

      /**
       * Get the used value for the specified property.
       */
      template<class PropSpec> typename PropSpec::used_value_type get() const
      {
        return PropSpec::get_used_value(PropSpec::get_value(compute_state.get_current_style()),
                                        *this);
      }

      template<class LengthPropSpec> bool is_auto_comp_len() const
      {
        return LengthPropSpec::get_value(compute_state.get_current_style()).state ==
          LengthPropSpec::value_type::state_Auto;
      }

      template<class LengthPropSpec> bool is_abs_comp_len() const
      {
        return LengthPropSpec::get_value(compute_state.get_current_style()).state ==
          LengthPropSpec::value_type::state_Abs;
      }

      StyleComputeState &get_compute_state() { return compute_state; }


      enum RelType
      {
        /**
         * Relative to nothing. Can be used for properties which doe
         * not support relative values at all.
         */
        rel_type_Zero,

        /**
         * Relative to the current font size.
         */
        rel_type_FontSize,

        /**
         * Relative to width of containing block. Drops to zero when
         * in shrink-to-fit mode.
         */
        rel_type_ContBlockWidth,

        /**
         * Relative to height of containing block. Drops to zero if
         * height of containing block is unknown.
         */
        rel_type_ContBlockHeight,

        /**
         * Same as rel_type_ContBlockWidth, but does not drop to zero
         * when in shrink-to-fit mode.
         */
        rel_type_Width,

        /**
         * Same as rel_type_ContBlockHeight, but does not drop to
         * zero. Produces a used value of -1 if height of containing
         * block is unknown.
         */
        rel_type_Height
      };


      double get_relative_base(RelType rel_type) const
      {
        switch (rel_type) {
        case rel_type_Zero:
          break;
        case rel_type_FontSize:
          return current_font_size;
        case rel_type_ContBlockWidth:
          return cont_block_shrinks_to_fit ? 0 : cont_block->width;
        case rel_type_ContBlockHeight:
          return cont_block->height < 0 ? 0 : cont_block->height;
        case rel_type_Width:
          return cont_block->width;
        case rel_type_Height:
          return cont_block->height < 0 ? -1 : cont_block->height;
        }
        return 0;
      }

      double get_current_font_size()
      {
        return current_font_size;
      }

      // FIXME: Should this not be cached in the StyleComputeState rather than here?
      double get_current_height_of_x()
      {
        if (!has_current_height_of_x) {
          current_height_of_x = compute_state.determine_height_of_x();
          has_current_height_of_x = true;
        }
        return current_height_of_x;
      }

      double get_dpcm()
      {
        return compute_state.get_dpcm();
      }

      void get_system_color(SystemColor, ComputedStyle::Color &color)
      {
        color = Util::Color::silver; // FIXME: Implement this!
      }

      double get_std_border_width(NamedBorderWidth w) const
      {
        return StyleComputeContext::get_std_border_width(w);
      }

      double get_std_font_size(int i)
      {
        return StyleComputeContext::get_std_font_size(i);
      }

      double increase_font_size(double s)
      {
        return StyleComputeContext::increase_font_size(s);
      }

      double decrease_font_size(double s)
      {
        return StyleComputeContext::decrease_font_size(s);
      }


      // Determine the value of the specified property that applies to
      // the parent element. For the root element, the default value
      // is returned.
      template<class PropSpec> typename PropSpec::value_get_type get_from_parent()
      {
        StyleGroup const group = PropSpec::comp_group;
        StyleComputeState::GroupBits const group_bit = 1ul<<group;
        ComputedStyle const &origin =
          PropSpec::is_default_inherited || parent && (parent->dirty & group_bit) ?
          (dirty & group_bit ? *backup_style : compute_state.get_current_style()) :
          compute_state.get_default_style();
        return PropSpec::get_value(origin);
      }


      template<class PropSpec> void inherit()
      {
        set<PropSpec>(get_from_parent<PropSpec>());
      }


      template<class PropSpec> void set(typename PropSpec::value_type const &value)
      {
        ComputedStyle &style = compute_state.get_current_style();
        typename PropSpec::value_type &prop = PropSpec::get_access(style);
        bool const default_inherited = PropSpec::is_default_inherited;
        if (default_inherited && prop == value) return;
        StyleGroup const group = PropSpec::comp_group;
        StyleComputeState::GroupBits const group_bit = 1ul<<group;
        if (!(dirty & group_bit)) {
          backup_group<PropSpec>();
          dirty |= group_bit;
        }
        prop = value;
        PropSpec::on_value_specified(style);
      }


      StyleApplyee(StyleComputeState *c); // For the viewport

      StyleApplyee(ContainingBlock *cont_block, bool cont_block_shrinks_to_fit,
                   StyledElement *e, StyleApplyee const *p);


      ~StyleApplyee()
      {
        if (!dirty) return;

        ComputedStyle &current = compute_state.get_current_style();
        if (dirty & 1ul<<style_group_Font) {
          current.font = backup_style->font;
        }
        if (dirty & 1ul<<style_group_Text) {
          current.text = backup_style->text;
        }
        if (dirty & 1ul<<style_group_Background) {
          current.background = backup_style->background;
        }
        if (dirty & 1ul<<style_group_Border) {
          current.border = backup_style->border;
        }
        if (dirty & 1ul<<style_group_Margin) {
          current.margin = backup_style->margin;
        }
        if (dirty & 1ul<<style_group_Padding) {
          current.padding = backup_style->padding;
        }
        if (dirty & 1ul<<style_group_Size) {
          current.size = backup_style->size;
        }
        if (dirty & 1ul<<style_group_Structure) {
          current.structure = backup_style->structure;
        }

        compute_state.accum_dirty_bits |= dirty & default_inherited_groups;
      }


    private:
      template<class PropSpec> void backup_group()
      {
        if (!backup_style) backup_style.reset(new ComputedStyle);
        ComputedStyle &current = compute_state.get_current_style();
        bool const set_to_default = !PropSpec::is_default_inherited;
        switch(PropSpec::comp_group) {
        case style_group_Font:
          backup_style->font = current.font;
          if (set_to_default) current.font = compute_state.get_default_style().font;
          break;
        case style_group_Text:
          backup_style->text = current.text;
          if (set_to_default) current.text = compute_state.get_default_style().text;
          break;
        case style_group_Background:
          backup_style->background = current.background;
          if (set_to_default) current.background = compute_state.get_default_style().background;
          break;
        case style_group_Border:
          backup_style->border = current.border;
          if (set_to_default) current.border = compute_state.get_default_style().border;
          break;
        case style_group_Margin:
          backup_style->margin = current.margin;
          if (set_to_default) current.margin = compute_state.get_default_style().margin;
          break;
        case style_group_Padding:
          backup_style->padding = current.padding;
          if (set_to_default) current.padding = compute_state.get_default_style().padding;
          break;
        case style_group_Size:
          backup_style->size = current.size;
          if (set_to_default) current.size = compute_state.get_default_style().size;
          break;
        case style_group_Structure:
          backup_style->structure = current.structure;
          if (set_to_default) current.structure = compute_state.get_default_style().structure;
          break;
        }
      }

/*
      void dump_border(ComputedStyle const &s, std::string msg)
      {
        std::cerr << msg<< ": border-width { top: " << s.border.top.width << "; right: " << s.border.right.width << "; bottom: " << s.border.bottom.width << "; left: " << s.border.left.width << "; }" << std::endl;
        std::cerr << msg<< ": border-style { top: " << s.border.top.style << "; right: " << s.border.right.style << "; bottom: " << s.border.bottom.style << "; left: " << s.border.left.style << "; }" << std::endl;
        std::cerr << msg<< ": border-color { top: " << s.border.top.color << "; right: " << s.border.right.color << "; bottom: " << s.border.bottom.color << "; left: " << s.border.left.color << "; }" << std::endl;
      }


      void dump_size(ComputedStyle const &s, std::string msg)
      {
        std::cerr << msg<< ": size { width: " << s.size.width << "; height: " << s.size.height << "; }" << std::endl;
      }
*/


      ContainingBlock *const cont_block;
      bool cont_block_shrinks_to_fit;
      StyleComputeState &compute_state;
      StyleApplyee const *const parent;
      static StyleComputeState::GroupBits const default_inherited_groups =
        1ul<<style_group_Font | 1ul<<style_group_Text;
      StyleComputeState::GroupBits dirty; // Groups that were changed by the style of the current element
      StyleComputeState::GroupBits accum_dirty;
      core::UniquePtr<ComputedStyle> backup_style;
      bool has_current_height_of_x;
      double current_height_of_x;

      // This is to hold its value fixed while the font style is applied.
      double current_font_size;
    };






    // Defininition of endowed enumerations


    struct EmptyEnumSpec { static core::EnumAssoc map[]; };
    enum EmptyBaseEnum {};
    typedef core::Enum<EmptyBaseEnum, EmptyEnumSpec> EmptyEnum;


    struct LengthUnitSpec { static core::EnumAssoc map[]; };
    typedef core::Enum<LengthUnit, LengthUnitSpec> LengthUnitEnum;



    struct NamedBorderWidthSpec { static core::EnumAssoc map[]; };
    typedef core::Enum<NamedBorderWidth, NamedBorderWidthSpec> NamedBorderWidthEnum;



    struct BorderStyleSpec { static core::EnumAssoc map[]; };
    typedef core::Enum<BorderStyle, BorderStyleSpec> BorderStyleEnum;



    struct FontStyleSpec { static core::EnumAssoc map[]; };
    typedef core::Enum<FontStyle, FontStyleSpec> FontStyleEnum;



    struct FontVariantSpec { static core::EnumAssoc map[]; };
    typedef core::Enum<FontVariant, FontVariantSpec> FontVariantEnum;



    struct FontWeightSpec { static core::EnumAssoc map[]; };
    typedef core::Enum<FontWeight, FontWeightSpec> FontWeightEnum;

    struct SpecialFontWeightSpec { static core::EnumAssoc map[]; };
    typedef core::Enum<SpecialFontWeight, SpecialFontWeightSpec> SpecialFontWeightEnum;



    struct NamedFontSizeSpec { static core::EnumAssoc map[]; };
    typedef core::Enum<NamedFontSize, NamedFontSizeSpec> NamedFontSizeEnum;



    struct SystemColorSpec { static core::EnumAssoc map[]; };
    typedef core::Enum<SystemColor, SystemColorSpec> SystemColorEnum;



    struct ClearValueSpec { static core::EnumAssoc map[]; };
    typedef core::Enum<ClearValue, ClearValueSpec> ClearValueEnum;



    struct DisplayValueSpec { static core::EnumAssoc map[]; };
    typedef core::Enum<DisplayValue, DisplayValueSpec> DisplayValueEnum;



    struct FloatValueSpec { static core::EnumAssoc map[]; };
    typedef core::Enum<FloatValue, FloatValueSpec> FloatValueEnum;



    struct OverflowValueSpec { static core::EnumAssoc map[]; };
    typedef core::Enum<OverflowValue, OverflowValueSpec> OverflowValueEnum;



    struct PositionValueSpec { static core::EnumAssoc map[]; };
    typedef core::Enum<PositionValue, PositionValueSpec> PositionValueEnum;




    struct PropBase
    {
      PropBase(): value_type(value_Unspecified) {}

    private:
      typedef int PropBase::*unspecified_bool_type;

    public:
      operator unspecified_bool_type() const throw()
      {
        return value_type != value_Unspecified ? &PropBase::value_type : 0;
      }

    protected:
      int value_type;
    };




    // The length unit with index I is represented as _value_End + I.
    // The named length with index I is represented as _value_End +
    // _lengthUnit_End + I.
    template<bool allow_bare_numbers, bool allow_percentages,
             bool allow_negative_values, bool has_keyword_auto,
             bool normal_instead_of_auto, class Names>
    struct LengthPropBase: PropBase
    {
      void format_value(dom::DOMString &out, StyleManipContext &ctx) const
      {
        switch (value_type) {
        case value_Unspecified: return;
        case value_Inherit: dom::str_append_port(out, "inherit"); return;
        case value_Auto:    dom::str_append_port(out, normal_instead_of_auto ?
                                                 "normal" : "auto"); return;
        }
        int const i = value_type - _value_End;
        int const j = i - _lengthUnit_End;
        if (value_type < _value_End || Names::num_names <= j)
          throw std::runtime_error("Unexpected type of value for length property");
        if (j < 0) {
          ctx.str_append_port(out, length);
          dom::str_append_port(out, LengthUnitEnum(LengthUnit(i)).str());
        }
        else {
          typedef typename Names::endowed_enum_type NameEnum;
          typedef typename NameEnum::base_enum_type Name;
          dom::str_append_port(out, NameEnum(Name(j)).str());
        }
      }

      bool parse_value(std::string const &str, StyleManipContext &ctx)
      {
        if (str.empty()) {
          value_type = value_Unspecified;
          return true;
        }
        if (str == "inherit") {
          value_type = value_Inherit;
          return true;
        }
        float l;
        std::string u;
        if (ctx.parse_length(str, l, u)) {
          LengthUnitEnum unit;
          if (unit.parse(u)) {
            if (!allow_bare_numbers && unit == lengthUnit_None && l != 0) return false;
            if (!allow_percentages && unit == lengthUnit_Percent) return false;
            if (!allow_negative_values && l < 0) return false;
            value_type = _value_End + unit;
            length = l;
            return true;
          }
          return false;
        }
        if (has_keyword_auto && str == (normal_instead_of_auto ? "normal" : "auto")) {
          value_type = value_Auto;
          return true;
        }
        if (0 < Names::num_names) {
          typedef typename Names::endowed_enum_type NameEnum;
//          typedef typename NameEnum::base_enum_type Name;
          NameEnum name;
          if (name.parse(str)) {
            value_type = _value_End + _lengthUnit_End + name;
            return true;
          }
        }
        return false;
      }

      bool operator==(LengthPropBase const &p) const
      {
        if (value_type != p.value_type) return false;
        int const i = this->value_type - _value_End;
        return 0 <= i && i < _lengthUnit_End ? length == p.length : true;
      }

    protected:
      float length;
    };




    template<class Spec>
    struct LengthProp:
      LengthPropBase<Spec::allow_bare_numbers, Spec::allow_percentages,
                     Spec::allow_negative_values, Spec::has_keyword_auto,
                     Spec::normal_instead_of_auto, typename Spec::Names>
    {
      typedef Spec spec_type;

      void apply_to(StyleApplyee &applyee) const
      {
        if (this->value_type == value_Unspecified) return;
        int const i = this->value_type - _value_End;
        int const j = i - _lengthUnit_End;
        typedef typename Spec::Names Names;
        typename Spec::value_type value = typename Spec::value_type();
        if (this->value_type < _value_End || Names::num_names <= j) {
          switch (this->value_type) {
          case value_Inherit: applyee.inherit<Spec>(); return;
          case value_Auto:    set_auto(value); break;
          default: throw std::runtime_error("Unexpected type of value for length property");
          }
        }
        else if (j < 0) {
          double const l = this->length;
          switch (LengthUnit(i)) {
          case lengthUnit_None:    set_bare_number(applyee, value, l);           break;
          case lengthUnit_Percent: set_percentage(applyee, value, l);            break;
          case lengthUnit_EM:      set_abs(value, from_font_size(applyee, l));   break;
          case lengthUnit_EX:      set_abs(value, from_height_of_x(applyee, l)); break;
          case lengthUnit_PX:      set_abs(value, l);                            break;
          case lengthUnit_CM:      set_abs(value, from_centimeters(applyee, l)); break;
          case lengthUnit_MM:      set_abs(value, from_millimeters(applyee, l)); break;
          case lengthUnit_IN:      set_abs(value, from_inches(applyee, l));      break;
          case lengthUnit_PT:      set_abs(value, from_points(applyee, l));      break;
          case lengthUnit_PC:      set_abs(value, from_picas(applyee, l));       break;
          case _lengthUnit_End: break; // Never happens
          }
        }
        else {
          typedef typename Names::endowed_enum_type::base_enum_type Name;
          set_abs(value, Names::get_named_value(applyee, Name(j)));
        }
        applyee.set<Spec>(value);
      }

      static double from_font_size(StyleApplyee &applyee, double font_sizes)
      {
        return applyee.get_current_font_size() * font_sizes;
      }

      static double from_height_of_x(StyleApplyee &applyee, double heights_of_x)
      {
        return applyee.get_current_height_of_x() * heights_of_x;
      }

      static double from_centimeters(StyleApplyee &applyee, double centimeters)
      {
        return applyee.get_dpcm() * centimeters;
      }

      static double from_millimeters(StyleApplyee &applyee, double millimeters)
      {
        return 0.1 * applyee.get_dpcm() * millimeters;
      }

      static double from_inches(StyleApplyee &applyee, double inches)
      {
        return 2.54 * applyee.get_dpcm() * inches;
      }

      static double from_points(StyleApplyee &applyee, double points)
      {
        return 2.54/72 * applyee.get_dpcm() * points;
      }

      static double from_picas(StyleApplyee &applyee, double picas)
      {
        return 12*2.54/72 * applyee.get_dpcm() * picas;
      }


      static void set_auto(ComputedStyle::Length &v)
      {
        v = 0;
      }

      static void set_auto(ComputedStyle::AugmentedLength &v)
      {
        v.state = ComputedStyle::AugmentedLength::state_Auto;
      }

      static void set_abs(ComputedStyle::Length &v, double w)
      {
        v = w;
      }

      static void set_abs(ComputedStyle::AugmentedLength &v, double w)
      {
        v.state = ComputedStyle::AugmentedLength::state_Abs;
        v.value = w;
      }

      static void set_percentage(StyleApplyee &applyee, ComputedStyle::Length &v, double w)
      {
        v = 0.01 * w * Spec::get_relative_base(applyee);
      }

      static void set_percentage(StyleApplyee &, ComputedStyle::AugmentedLength &v, double w)
      {
        if (Spec::force_percentage_comp) set_abs(v,w);
        else {
          v.state = ComputedStyle::AugmentedLength::state_Rel;
          v.value = 0.01 * w;
        }
      }

      static void set_bare_number(StyleApplyee &applyee, ComputedStyle::Length &v, double w)
      {
        v = w == 0 ? 0 : w * Spec::get_relative_base(applyee);
      }

      static void set_bare_number(StyleApplyee &, ComputedStyle::AugmentedLength &v, double w)
      {
        v.state = ComputedStyle::AugmentedLength::state_Rel;
        v.value = w;
      }

      void format(dom::DOMString &out, StyleManipContext &ctx) const
      {
        if (this->value_type == value_Unspecified) return;
        dom::str_append_port(out, Spec::get_name());
        dom::str_append_port(out, ": ");
        this->format_value(out, ctx);
        dom::str_append_port(out, "; ");
      }

      using LengthPropBase<Spec::allow_bare_numbers, Spec::allow_percentages,
                           Spec::allow_negative_values, Spec::has_keyword_auto,
                           Spec::normal_instead_of_auto, typename Spec::Names>::operator=;
    };




    // The enumeration keyword with index I is represented as
    // _value_End + SpecialNames::num_names + I. The special keyword
    // with index I is represented as _value_End + I.
    template<class Enum, class SpecialNames> struct EnumPropBase: PropBase
    {
      void format_value(dom::DOMString &out, StyleManipContext &) const
      {
        switch (value_type) {
        case value_Unspecified: return;
        case value_Inherit: dom::str_append_port(out, "inherit"); return;
        }
        if (value_type < _value_End)
          throw std::runtime_error("Unexpected type of value for enum property");
        int const i = value_type - _value_End;
        int const j = i - SpecialNames::num_names;
        if (j < 0) {
          typedef typename SpecialNames::endowed_enum_type NameEnum;
          typedef typename NameEnum::base_enum_type Name;
          dom::str_append_port(out, NameEnum(Name(i)).str());
        }
        else {
          typedef typename Enum::base_enum_type base_enum_type;
          dom::str_append_port(out, Enum(base_enum_type(j)).str());
        }
      }

      bool parse_value(std::string const &str, StyleManipContext &)
      {
        if (str.empty()) {
          value_type = value_Unspecified;
          return true;
        }
        if (str == "inherit") {
          value_type = value_Inherit;
          return true;
        }
        if (0 < SpecialNames::num_names) {
          typedef typename SpecialNames::endowed_enum_type NameEnum;
//          typedef typename NameEnum::base_enum_type Name;
          NameEnum name;
          if (name.parse(str)) {
            value_type = _value_End + name;
            return true;
          }
        }
        Enum value;
        if (value.parse(str)) {
          value_type = _value_End + SpecialNames::num_names + value;
          return true;
        }
        return false;
      }

      bool operator==(EnumPropBase const &p) const { return value_type == p.value_type; }
    };




    template<class Spec> struct EnumProp: EnumPropBase<typename Spec::endowed_enum_type,
                                                       typename Spec::SpecialNames>
    {
      typedef Spec spec_type;

      void apply_to(StyleApplyee &applyee) const
      {
        switch (this->value_type) {
        case value_Unspecified: return;
        case value_Inherit: applyee.inherit<Spec>(); return;
        }
        if (this->value_type < _value_End)
          throw std::runtime_error("Unexpected type of value for enum property");
        typedef typename Spec::endowed_enum_type::base_enum_type enum_type;
        enum_type value;
        typedef typename Spec::SpecialNames SpecialNames;
        int const i = this->value_type - _value_End;
        int const j = i - SpecialNames::num_names;
        if (j < 0) {
          typedef typename SpecialNames::endowed_enum_type::base_enum_type Name;
          value = SpecialNames::get_named_value(applyee, Name(i));
        }
        else value = static_cast<enum_type>(j);
        applyee.set<Spec>(value);
      }

      void format(dom::DOMString &out, StyleManipContext &ctx) const
      {
        if (this->value_type == value_Unspecified) return;
        dom::str_append_port(out, Spec::get_name());
        dom::str_append_port(out, ": ");
        this->format_value(out, ctx);
        dom::str_append_port(out, "; ");
      }

      using EnumPropBase<typename Spec::endowed_enum_type,
                         typename Spec::SpecialNames>::operator=;
    };




    // In CSS2.1 'background-color' has a special 'transparent' value
    // that the other color properties do not. In CSS3 'transparent'
    // is a genuine named color and is availble to all color
    // properties. The color keyword with index I is represented as
    // _value_End + I.
    template<bool has_css21_transparent> struct ColorPropBase: PropBase
    {
      void format_value(dom::DOMString &out, StyleManipContext &ctx) const
      {
        using Util::PackedTRGB;

        int format;
        switch (value_type) {
        case value_Unspecified: return;
        case value_Inherit:     dom::str_append_port(out, "inherit");     return;
        case value_Transparent: dom::str_append_port(out, "transparent"); return;
        case value_RGB_Number:  format = 3; break;
        case value_RGB_Percent: format = 4; break;
        case value_HSL_Number:  format = 5; break;
        case value_HSL_Percent: format = 6; break;
        default:
          {
            if (value_type < _value_End)
              throw std::runtime_error("Unexpected type of value for color property");
            int const i = value_type - _value_End;
            int const j = i - _sysColor_End;
            if (0 <= j) dom::str_append_port(out, PackedTRGB::get_color_name(j));
            else dom::str_append_port(out, SystemColorEnum(SystemColor(i)).str());
          }
          return;
        }
        std::string const str = PackedTRGB::format(format, Math::Vec4F(red, green, blue, alpha),
                                                   ctx.get_color_parser_css_level());
        dom::str_append_port(out, str);
      }

      bool parse_value(std::string const &str, StyleManipContext &ctx)
      {
        using Util::PackedTRGB;

        PackedTRGB::CssLevel const css_level = ctx.get_color_parser_css_level();
        if (str.empty()) {
          value_type = value_Unspecified;
          return true;
        }
        if (str == "inherit") {
          value_type = value_Inherit;
          return true;
        }
        if (has_css21_transparent && css_level == PackedTRGB::css21 && str == "transparent") {
          value_type = value_Transparent;
          return true;
        }
        int named_index;
        Math::Vec4F rgba;
        int const res = PackedTRGB::parse(str, named_index, rgba, css_level);
        switch (res) {
        case 0:
          {
            SystemColorEnum sys_color;
            if(sys_color.parse(str)) {
              value_type = _value_End + sys_color;
              if (css_level != PackedTRGB::css21)
                ctx.deprecation_warning(dom::str_from_port("System colors are "
                                                           "deprecated in CSS3"));
              return true;
            }
          }
          return false;
        case 1:
          value_type = _value_End + _sysColor_End + named_index;
          return true;
        case 2:
        case 3: value_type = value_RGB_Number;  break;
        case 4: value_type = value_RGB_Percent; break;
        case 5: value_type = value_HSL_Number;  break;
        case 6: value_type = value_HSL_Percent; break;
        default: throw std::runtime_error("Unexpected parsed color format");
        }
        red   = rgba[0];
        green = rgba[1];
        blue  = rgba[2];
        alpha = rgba[3];
        return true;
      }

      bool operator==(ColorPropBase const &p) const
      {
        if (value_type != p.value_type) return false;
        switch (value_type) {
        default: return true;
        case value_RGB_Number:
        case value_RGB_Percent:
        case value_HSL_Number:
        case value_HSL_Percent:
          return red == p.red && green == p.green && blue == p.blue && alpha == p.alpha;
        }
      }

    protected:
      float red, green, blue, alpha;
    };




    template<class Spec> struct ColorProp: ColorPropBase<Spec::has_css21_transparent>
    {
      typedef Spec spec_type;

      void apply_to(StyleApplyee &applyee) const
      {
        using Util::PackedTRGB;

        float r,g,b,a;
        switch (this->value_type) {
        case value_Unspecified: return;
        case value_Inherit: applyee.inherit<Spec>(); return;
        case value_Transparent: r = g = b = a = 0; break;
        case value_RGB_Number:
          r = this->red/255; g = this->green/255; b = this->blue/255; a = this->alpha; break;
        case value_RGB_Percent:
          r = this->red/100; g = this->green/100; b = this->blue/100; a = this->alpha; break;
        case value_HSL_Number:
        case value_HSL_Percent:
          throw std::runtime_error("Unfortunately, the HSL color space is not yet available");
        default:
          {
            if (this->value_type < _value_End)
              throw std::runtime_error("Unexpected type of value for color property");
            ComputedStyle::Color color;
            int const i = this->value_type - _value_End;
            int const j = i - _sysColor_End;
            if (0 <= j) color = PackedTRGB::get_named_color(j);
            else applyee.get_system_color(SystemColor(i), color);
            applyee.set<Spec>(color);
            return;
          }
        }
        applyee.set<Spec>(ComputedStyle::Color(r,g,b,a));
      }

      void format(dom::DOMString &out, StyleManipContext &ctx) const
      {
        if (this->value_type == value_Unspecified) return;
        dom::str_append_port(out, Spec::get_name());
        dom::str_append_port(out, ": ");
        this->format_value(out, ctx);
        dom::str_append_port(out, "; ");
      }

      using ColorPropBase<Spec::has_css21_transparent>::operator=;
    };








    // Compile-time specification of all CSS properties


    // A concrete style property specification is a class with the
    // following members:
    //
    //   static value_get_type get_value(ComputedStyle const &)
    //
    //   static value_type &get_access(ComputedStyle &)
    //
    // get_value() must return the computed value as defined by the
    // CSS specification. get_access() must give direct access to the
    // property as it is stored in an instance of ComputedStyle. The
    // stored value is generally identical to the computed value, but
    // there are exceptions.

    template<class T, bool def_inherit, StyleGroup group> struct PropSpecBase
    {
      // This type may be overridden by derived classes.
      typedef T value_type;

      // This type may be overridden by derived classes, but it must
      // always be convertible to 'value_type'.
      typedef value_type const &value_get_type;

      static bool const is_default_inherited = def_inherit;

      static StyleGroup const comp_group = group;

      // Called if this property is set to any value after it has
      // received its default value.
      static void on_value_specified(ComputedStyle &) {}
    };


    template<class T> struct NoNamedValues
    {
      static int const num_names = 0;
      typedef EmptyEnum endowed_enum_type;
      static T get_named_value(StyleApplyee &, EmptyBaseEnum) { return T(); }
    };

    template<bool def_inherit, StyleGroup group,
             StyleApplyee::RelType rel_type = StyleApplyee::rel_type_Zero>
    struct LengthPropSpec: PropSpecBase<ComputedStyle::Length, def_inherit, group>
    {
      static bool const allow_bare_numbers = false;

      static bool const allow_percentages = false;

      // When the computed value is requested, a percentage is
      // converted to an absolute number of pixels if, and only if
      // this flag is true or 'value_type' is ComputedStyle::Length.
      static bool const force_percentage_comp = false;

      // This one is used to resolve bare numbers when they are
      // allowed and 'value_type' is ComputedStyle::Length. It is
      // also used to resolve percentages when they are allowed and
      // 'value_type' is ComputedStyle::Length or
      // force_percentage_comp is true.
      static double get_relative_base(StyleApplyee const &a)
      {
        return a.get_relative_base(rel_type);
      }

      static bool const allow_negative_values = false;

      static bool const has_keyword_auto = false;

      static bool const normal_instead_of_auto = false;

      typedef NoNamedValues<double> Names;

      typedef int used_value_type;

      // For now, we always round towards zero. This seems to be in
      // agreement with WebKit and Presto, but not Gecko, which
      // appears to not round at all.
      static int get_used_value(ComputedStyle::Length v, StyleApplyee const &) { return int(v); }
    };

    template<bool def_inherit, StyleGroup group,
             StyleApplyee::RelType rel_type = StyleApplyee::rel_type_Zero>
    struct AugmentedLengthPropSpec: LengthPropSpec<def_inherit, group, rel_type>
    {
      typedef ComputedStyle::AugmentedLength value_type;
      typedef value_type const &value_get_type;
      static bool const allow_percentages = true;

      // For now, we always round towards zero. This seems to be in
      // agreement with WebKit and Presto, but not Gecko, which
      // appears to not round at all.
      static int get_used_value(value_get_type v, StyleApplyee const &a)
      {
        if (v.state == value_type::state_Auto) return -1;
        double const d = v.state == value_type::state_Abs ? v.value :
          AugmentedLengthPropSpec::get_relative_base(a)*v.value;
        return int(d);
      }
    };

    template<class Enum, bool def_inherit, StyleGroup group>
    struct EnumPropSpec:
      PropSpecBase<typename Enum::base_enum_type, def_inherit, group>
    {
      typedef Enum endowed_enum_type;
      typedef NoNamedValues<typename Enum::base_enum_type> SpecialNames;

      typedef typename Enum::base_enum_type used_value_type;

      static used_value_type get_used_value(typename Enum::base_enum_type v, StyleApplyee const &)
      {
        return v;
      }
    };

    template<bool def_inherit, StyleGroup group>
    struct ColorPropSpec:
      PropSpecBase<ComputedStyle::Color, def_inherit, group>
    {
      static bool const has_css21_transparent = false;

      typedef Util::PackedTRGB used_value_type;

      static used_value_type get_used_value(ComputedStyle::Color v, StyleApplyee const &)
      {
        return v;
      }
    };



    struct PropSpec_BackgroundColor: ColorPropSpec<false, style_group_Background>
    {
      static bool const has_css21_transparent = true;
      static std::string get_name() { return "background-color"; }

      static value_get_type get_value(ComputedStyle const &s) { return s.background.color; }
      static value_type &get_access(ComputedStyle &s)         { return s.background.color; }
    };



    struct BorderWidthPropSpecBase: LengthPropSpec<false, style_group_Border>
    {
      typedef value_type value_get_type;

      struct Names
      {
        static int const num_names = _borderWidth_End;
        typedef NamedBorderWidthEnum endowed_enum_type;
        static double get_named_value(StyleApplyee &applyee, NamedBorderWidth w)
        {
          return applyee.get_std_border_width(w);
        }
      };
    };

    template<ComputedStyle::Border::Side ComputedStyle::Border::*side>
    struct BorderWidthPropSpec: BorderWidthPropSpecBase
    {
      static value_get_type get_value(ComputedStyle const &s)
      {
        ComputedStyle::Border::Side const &side2 = s.border.*side;
        return side2.style == borderStyle_None ||
          side2.style == borderStyle_Hidden ? 0 : side2.width;
      }

      static value_type &get_access(ComputedStyle &s)
      {
        return (s.border.*side).width;
      }
    };

    struct PropSpec_BorderTopWidth: BorderWidthPropSpec<&ComputedStyle::Border::top>
    {
      static std::string get_name() { return "border-top-width"; }
    };

    struct PropSpec_BorderRightWidth: BorderWidthPropSpec<&ComputedStyle::Border::right>
    {
      static std::string get_name() { return "border-right-width"; }
    };

    struct PropSpec_BorderBottomWidth: BorderWidthPropSpec<&ComputedStyle::Border::bottom>
    {
      static std::string get_name() { return "border-bottom-width"; }
    };

    struct PropSpec_BorderLeftWidth: BorderWidthPropSpec<&ComputedStyle::Border::left>
    {
      static std::string get_name() { return "border-left-width"; }
    };



    typedef EnumPropSpec<BorderStyleEnum, false, style_group_Border> BorderStylePropSpecBase;

    template<ComputedStyle::Border::Side ComputedStyle::Border::*side>
    struct BorderStylePropSpec: BorderStylePropSpecBase
    {
      static value_get_type get_value(ComputedStyle const &s) { return (s.border.*side).style; }
      static value_type &get_access(ComputedStyle &s)         { return (s.border.*side).style; }
    };

    struct PropSpec_BorderTopStyle: BorderStylePropSpec<&ComputedStyle::Border::top>
    {
      static std::string get_name() { return "border-top-style"; }
    };

    struct PropSpec_BorderRightStyle: BorderStylePropSpec<&ComputedStyle::Border::right>
    {
      static std::string get_name() { return "border-right-style"; }
    };

    struct PropSpec_BorderBottomStyle: BorderStylePropSpec<&ComputedStyle::Border::bottom>
    {
      static std::string get_name() { return "border-bottom-style"; }
    };

    struct PropSpec_BorderLeftStyle: BorderStylePropSpec<&ComputedStyle::Border::left>
    {
      static std::string get_name() { return "border-left-style"; }
    };



    typedef ColorPropSpec<false, style_group_Border> BorderColorPropSpecBase;

    template<ComputedStyle::Border::Side ComputedStyle::Border::*side>
    struct BorderColorPropSpec: BorderColorPropSpecBase
    {
      static value_get_type get_value(ComputedStyle const &s)
      {
        ComputedStyle::Border::Side const &side2 = s.border.*side;
        return side2.color_specified ? side2.color : s.text.color;
      }

      static value_type &get_access(ComputedStyle &s)
      {
        return (s.border.*side).color;
      }

      static void on_value_specified(ComputedStyle &s)
      {
        (s.border.*side).color_specified = true;
      }
    };

    struct PropSpec_BorderTopColor: BorderColorPropSpec<&ComputedStyle::Border::top>
    {
      static std::string get_name() { return "border-top-color"; }
    };

    struct PropSpec_BorderRightColor: BorderColorPropSpec<&ComputedStyle::Border::right>
    {
      static std::string get_name() { return "border-right-color"; }
    };

    struct PropSpec_BorderBottomColor: BorderColorPropSpec<&ComputedStyle::Border::bottom>
    {
      static std::string get_name() { return "border-bottom-color"; }
    };

    struct PropSpec_BorderLeftColor: BorderColorPropSpec<&ComputedStyle::Border::left>
    {
      static std::string get_name() { return "border-left-color"; }
    };



    struct PropSpec_Color: ColorPropSpec<true, style_group_Text>
    {
      static std::string get_name() { return "color"; }

      static value_get_type get_value(ComputedStyle const &s) { return s.text.color; }
      static value_type &get_access(ComputedStyle &s)         { return s.text.color; }
    };



    struct PropSpec_FontStyle: EnumPropSpec<FontStyleEnum, true, style_group_Font>
    {
      static std::string get_name() { return "font-style"; }

      static value_get_type get_value(ComputedStyle const &s) { return s.font.style; }
      static value_type &get_access(ComputedStyle &s)         { return s.font.style; }
    };



    struct PropSpec_FontVariant: EnumPropSpec<FontVariantEnum, true, style_group_Font>
    {
      static std::string get_name() { return "font-variant"; }

      static value_get_type get_value(ComputedStyle const &s) { return s.font.variant; }
      static value_type &get_access(ComputedStyle &s)         { return s.font.variant; }
    };



    struct PropSpec_FontWeight: EnumPropSpec<FontWeightEnum, true, style_group_Font>
    {
      struct SpecialNames
      {
        static int const num_names = _specialFontWeight_End;
        typedef SpecialFontWeightEnum endowed_enum_type;
        static FontWeight get_named_value(StyleApplyee &applyee, SpecialFontWeight w)
        {
          switch (w) {
          default:                     return fontWeight_400;
          case specialFontWeight_Bold: return fontWeight_700;

          case specialFontWeight_Bolder:
            switch (applyee.get_from_parent<PropSpec_FontWeight>()) {
            case fontWeight_100: return fontWeight_400;
            case fontWeight_200: return fontWeight_400;
            case fontWeight_300: return fontWeight_400;
            default:             return fontWeight_700;
            case fontWeight_500: return fontWeight_700;
            case fontWeight_600: return fontWeight_900;
            case fontWeight_700: return fontWeight_900;
            case fontWeight_800: return fontWeight_900;
            case fontWeight_900: return fontWeight_900;
            }

          case specialFontWeight_Lighter:
            switch (applyee.get_from_parent<PropSpec_FontWeight>()) {
            case fontWeight_100: return fontWeight_100;
            case fontWeight_200: return fontWeight_100;
            case fontWeight_300: return fontWeight_100;
            default:             return fontWeight_100;
            case fontWeight_500: return fontWeight_100;
            case fontWeight_600: return fontWeight_400;
            case fontWeight_700: return fontWeight_400;
            case fontWeight_800: return fontWeight_700;
            case fontWeight_900: return fontWeight_700;
            }
          }
        }
      };

      static std::string get_name() { return "font-weight"; }

      static value_get_type get_value(ComputedStyle const &s)
      {
        return s.font.weight;
      }

      static value_type &get_access(ComputedStyle &s)
      {
        return s.font.weight;
      }
    };



    struct PropSpec_FontSize:
      LengthPropSpec<true, style_group_Font, StyleApplyee::rel_type_FontSize>
    {
      static bool const allow_percentages = true;

      struct Names
      {
        static int const num_names = _fontSize_End;
        typedef NamedFontSizeEnum endowed_enum_type;
        static double get_named_value(StyleApplyee &applyee, NamedFontSize s)
        {
          switch (s) {
          case fontSize_XXSmall: return applyee.get_std_font_size(-3);
          case fontSize_XSmall:  return applyee.get_std_font_size(-2);
          case fontSize_Small:   return applyee.get_std_font_size(-1);
          default:               return applyee.get_std_font_size(00);
          case fontSize_Large:   return applyee.get_std_font_size(+1);
          case fontSize_XLarge:  return applyee.get_std_font_size(+2);
          case fontSize_XXLarge: return applyee.get_std_font_size(+3);

          case fontSize_Larger:
            return applyee.increase_font_size(applyee.get_current_font_size());

          case fontSize_Smaller:
            return applyee.decrease_font_size(applyee.get_current_font_size());
          }
        }
      };

      static std::string get_name() { return "font-size"; }

      static value_get_type get_value(ComputedStyle const &s)
      {
        return s.font.size;
      }

      static value_type &get_access(ComputedStyle &s)
      {
        return s.font.size;
      }
    };



    struct PropSpec_LineHeight:
      AugmentedLengthPropSpec<true, style_group_Text, StyleApplyee::rel_type_FontSize>
    {
      static bool const allow_bare_numbers     = true;
      static bool const allow_percentages      = true;
      static bool const force_percentage_comp  = true;
      static bool const has_keyword_auto       = true;
      static bool const normal_instead_of_auto = true;

      static std::string get_name() { return "line-height"; }

      static value_get_type get_value(ComputedStyle const &s)
      {
        return s.text.line_height;
      }

      static value_type &get_access(ComputedStyle &s)
      {
        return s.text.line_height;
      }
    };



    template<ComputedStyle::AugmentedLength ComputedStyle::Margin::*side>
    struct MarginPropSpec:
      AugmentedLengthPropSpec<false, style_group_Margin, StyleApplyee::rel_type_ContBlockWidth>
    {
      static bool const allow_percentages     = true;
      static bool const allow_negative_values = true;
      static bool const has_keyword_auto      = true;

      static value_get_type get_value(ComputedStyle const &s) { return s.margin.*side; }
      static value_type &get_access(ComputedStyle &s)         { return s.margin.*side; }
    };

    struct PropSpec_MarginTop: MarginPropSpec<&ComputedStyle::Margin::top>
    {
      static std::string get_name() { return "margin-top"; }
    };

    struct PropSpec_MarginRight: MarginPropSpec<&ComputedStyle::Margin::right>
    {
      static std::string get_name() { return "margin-right"; }
    };

    struct PropSpec_MarginBottom: MarginPropSpec<&ComputedStyle::Margin::bottom>
    {
      static std::string get_name() { return "margin-bottom"; }
    };

    struct PropSpec_MarginLeft: MarginPropSpec<&ComputedStyle::Margin::left>
    {
      static std::string get_name() { return "margin-left"; }
    };



    template<ComputedStyle::AugmentedLength ComputedStyle::Padding::*side>
    struct PaddingPropSpec:
      AugmentedLengthPropSpec<false, style_group_Padding, StyleApplyee::rel_type_ContBlockWidth>
    {
      static bool const allow_percentages = true;

      static value_get_type get_value(ComputedStyle const &s) { return s.padding.*side; }
      static value_type &get_access(ComputedStyle &s)         { return s.padding.*side; }
    };

    struct PropSpec_PaddingTop: PaddingPropSpec<&ComputedStyle::Padding::top>
    {
      static std::string get_name() { return "padding-top"; }
    };

    struct PropSpec_PaddingRight: PaddingPropSpec<&ComputedStyle::Padding::right>
    {
      static std::string get_name() { return "padding-right"; }
    };

    struct PropSpec_PaddingBottom: PaddingPropSpec<&ComputedStyle::Padding::bottom>
    {
      static std::string get_name() { return "padding-bottom"; }
    };

    struct PropSpec_PaddingLeft: PaddingPropSpec<&ComputedStyle::Padding::left>
    {
      static std::string get_name() { return "padding-left"; }
    };



    template<ComputedStyle::AugmentedLength ComputedStyle::Size::*which,
             StyleApplyee::RelType rel_type>
    struct SizePropSpec: AugmentedLengthPropSpec<false, style_group_Size, rel_type>
    {
      static bool const allow_percentages = true;
      static bool const has_keyword_auto  = true;

      static ComputedStyle::AugmentedLength const &get_value(ComputedStyle const &s)
      {
        return s.size.*which;
      }

      static ComputedStyle::AugmentedLength &get_access(ComputedStyle &s)
      {
        return s.size.*which;
      }
    };

    struct PropSpec_Width:
      SizePropSpec<&ComputedStyle::Size::width, StyleApplyee::rel_type_Width>
    {
      static std::string get_name() { return "width"; }
    };

    struct PropSpec_Height:
      SizePropSpec<&ComputedStyle::Size::height, StyleApplyee::rel_type_Height>
    {
      static std::string get_name() { return "height"; }
    };



    struct PropSpec_Clear: EnumPropSpec<ClearValueEnum, false, style_group_Structure>
    {
      static std::string get_name() { return "clear"; }

      static value_get_type get_value(ComputedStyle const &s) { return s.structure.clear; }
      static value_type &get_access(ComputedStyle &s)         { return s.structure.clear; }
    };

    struct PropSpec_Display: EnumPropSpec<DisplayValueEnum, false, style_group_Structure>
    {
      static std::string get_name() { return "display"; }

      static value_get_type get_value(ComputedStyle const &s) { return s.structure.display; }
      static value_type &get_access(ComputedStyle &s)         { return s.structure.display; }
    };

    struct PropSpec_Float: EnumPropSpec<FloatValueEnum, false, style_group_Structure>
    {
      static std::string get_name() { return "float"; }

      static value_get_type get_value(ComputedStyle const &s) { return s.structure.cssFloat; }
      static value_type &get_access(ComputedStyle &s)         { return s.structure.cssFloat; }
    };

    struct PropSpec_Overflow: EnumPropSpec<OverflowValueEnum, false, style_group_Structure>
    {
      static std::string get_name() { return "overflow"; }

      static value_get_type get_value(ComputedStyle const &s) { return s.structure.overflow; }
      static value_type &get_access(ComputedStyle &s)         { return s.structure.overflow; }
    };

    struct PropSpec_Position: EnumPropSpec<PositionValueEnum, false, style_group_Structure>
    {
      static std::string get_name() { return "position"; }

      static value_get_type get_value(ComputedStyle const &s) { return s.structure.position; }
      static value_type &get_access(ComputedStyle &s)         { return s.structure.position; }
    };







    // Definition of a dynamic style information


    template<class Top, class Right, class Bottom, class Left> struct RectGroup
    {
      void apply_to(StyleApplyee &applyee) const
      {
        top.apply_to(applyee);
        right.apply_to(applyee);
        bottom.apply_to(applyee);
        left.apply_to(applyee);
      }

      void format(dom::DOMString &out, StyleManipContext &ctx) const
      {
        top.format(out, ctx);
        right.format(out, ctx);
        bottom.format(out, ctx);
        left.format(out, ctx);
      }

      Top    top;
      Right  right;
      Bottom bottom;
      Left   left;
    };




    struct BackgroundGroup
    {
      void apply_to(StyleApplyee &applyee) const
      {
        color.apply_to(applyee);
      }

      bool format_shorthand(dom::DOMString &, StyleManipContext &) const
      {
        return false; // FIXME: Implement this!
      }

      void format(dom::DOMString &out, StyleManipContext &ctx) const
      {
        if (format_shorthand(out, ctx)) return;
        color.format(out, ctx);
      }

      ColorProp<PropSpec_BackgroundColor> color;
    };




    struct BorderGroup
    {
      void apply_to(StyleApplyee &applyee) const
      {
        width.apply_to(applyee);
        style.apply_to(applyee);
        color.apply_to(applyee);
      }

      bool format_shorthand(dom::DOMString &out, StyleManipContext &ctx) const
      {
        TopWidth const w = width.top;
        if (w != width.right || w != width.bottom || w != width.left) return false;
        TopStyle const s = style.top;
        if (s != style.right || s != style.bottom || s != style.left) return false;
        TopColor const c = color.top;
        if (c != color.right || c != color.bottom || c != color.left) return false;
        if (!w && !s && !c) return false;
        dom::str_append_port(out, "border:");
        if (w) {
          dom::str_append_port(out, " ");
          w.format_value(out, ctx);
        }
        if (s) {
          dom::str_append_port(out, " ");
          s.format_value(out, ctx);
        }
        if (c) {
          dom::str_append_port(out, " ");
          c.format_value(out, ctx);
        }
        dom::str_append_port(out, "; ");
        return true;
      }

      void format(dom::DOMString &out, StyleManipContext &ctx) const
      {
        if (format_shorthand(out, ctx)) return;
        width.format(out, ctx);
        style.format(out, ctx);
        color.format(out, ctx);
      }

      typedef LengthProp<PropSpec_BorderTopWidth>    TopWidth;
      typedef LengthProp<PropSpec_BorderRightWidth>  RightWidth;
      typedef LengthProp<PropSpec_BorderBottomWidth> BottomWidth;
      typedef LengthProp<PropSpec_BorderLeftWidth>   LeftWidth;
      typedef RectGroup<TopWidth, RightWidth, BottomWidth, LeftWidth> Width;
      Width width;

      typedef EnumProp<PropSpec_BorderTopStyle>    TopStyle;
      typedef EnumProp<PropSpec_BorderRightStyle>  RightStyle;
      typedef EnumProp<PropSpec_BorderBottomStyle> BottomStyle;
      typedef EnumProp<PropSpec_BorderLeftStyle>   LeftStyle;
      typedef RectGroup<TopStyle, RightStyle, BottomStyle, LeftStyle> Style;
      Style style;

      typedef ColorProp<PropSpec_BorderTopColor>    TopColor;
      typedef ColorProp<PropSpec_BorderRightColor>  RightColor;
      typedef ColorProp<PropSpec_BorderBottomColor> BottomColor;
      typedef ColorProp<PropSpec_BorderLeftColor>   LeftColor;
      typedef RectGroup<TopColor, RightColor, BottomColor, LeftColor> Color;
      Color color;
    };




    struct FontGroup
    {
      void apply_font_to(StyleApplyee &applyee) const
      {
        style.apply_to(applyee);
        variant.apply_to(applyee);
        weight.apply_to(applyee);
        size.apply_to(applyee);
        // FIXME: This one must also apply 'font-family'
      }

      void apply_to(StyleApplyee &applyee) const
      {
        line_height.apply_to(applyee);
      }

      bool format_shorthand(dom::DOMString &, StyleManipContext &) const
      {
        return false; // FIXME: Implement this!
      }

      void format(dom::DOMString &out, StyleManipContext &ctx) const
      {
        if (format_shorthand(out, ctx)) return;
        size.format(out, ctx);
        line_height.format(out, ctx);
      }

      EnumProp<PropSpec_FontStyle>    style;
      EnumProp<PropSpec_FontVariant>  variant;
      EnumProp<PropSpec_FontWeight>   weight;
      LengthProp<PropSpec_FontSize>   size;  // FIXME: What about special font size keywords?
      LengthProp<PropSpec_LineHeight> line_height;
    };




    typedef LengthProp<PropSpec_MarginTop>    MarginTop;    // FIXME: What about keyword 'auto'?
    typedef LengthProp<PropSpec_MarginRight>  MarginRight;
    typedef LengthProp<PropSpec_MarginBottom> MarginBottom;
    typedef LengthProp<PropSpec_MarginLeft>   MarginLeft;
    typedef RectGroup<MarginTop, MarginRight, MarginBottom, MarginLeft> MarginRect;
    struct MarginGroup: MarginRect
    {
      bool format_shorthand(dom::DOMString &, StyleManipContext &) const
      {
        return false; // FIXME: Implement this!
      }

      void format(dom::DOMString &out, StyleManipContext &ctx) const
      {
        if (format_shorthand(out, ctx)) return;
        MarginRect::format(out, ctx);
      }
    };




    typedef LengthProp<PropSpec_PaddingTop>    PaddingTop;
    typedef LengthProp<PropSpec_PaddingRight>  PaddingRight;
    typedef LengthProp<PropSpec_PaddingBottom> PaddingBottom;
    typedef LengthProp<PropSpec_PaddingLeft>   PaddingLeft;
    typedef RectGroup<PaddingTop, PaddingRight, PaddingBottom, PaddingLeft> PaddingRect;
    struct PaddingGroup: PaddingRect
    {
      bool format_shorthand(dom::DOMString &, StyleManipContext &) const
      {
        return false; // FIXME: Implement this!
      }

      void format(dom::DOMString &out, StyleManipContext &ctx) const
      {
        if (format_shorthand(out, ctx)) return;
        PaddingRect::format(out, ctx);
      }
    };




    struct SizeGroup
    {
      void apply_to(StyleApplyee &applyee) const
      {
        width.apply_to(applyee);
        height.apply_to(applyee);
      }

      void format(dom::DOMString &out, StyleManipContext &ctx) const
      {
        width.format(out, ctx);
        height.format(out, ctx);
      }

      LengthProp<PropSpec_Width>  width;
      LengthProp<PropSpec_Height> height;
    };




    struct StructureGroup
    {
      void apply_to(StyleApplyee &applyee) const
      {
        clear.apply_to(applyee);
        display.apply_to(applyee);
        cssFloat.apply_to(applyee);
        overflow.apply_to(applyee);
        position.apply_to(applyee);
      }

      void format(dom::DOMString &out, StyleManipContext &ctx) const
      {
        clear.format(out, ctx);
        display.format(out, ctx);
        cssFloat.format(out, ctx);
        overflow.format(out, ctx);
        position.format(out, ctx);
      }

      EnumProp<PropSpec_Clear>    clear;
      EnumProp<PropSpec_Display>  display;
      EnumProp<PropSpec_Float>    cssFloat;
      EnumProp<PropSpec_Overflow> overflow;
      EnumProp<PropSpec_Position> position;
    };




    struct TextGroup
    {
      void apply_font_to(StyleApplyee &applyee) const
      {
        if (font) font->apply_font_to(applyee);
      }

      void apply_to(StyleApplyee &applyee) const
      {
        color.apply_to(applyee);
        if (font) font->apply_to(applyee);
      }

      void format(dom::DOMString &out, StyleManipContext &ctx) const
      {
        color.format(out, ctx);
        if (font) font->format(out, ctx);
      }

      ColorProp<PropSpec_Color>  color;
      core::UniquePtr<FontGroup> font;
    };




    struct BoxGroup1
    {
      void apply_to(StyleApplyee &applyee) const
      {
        if (background) background->apply_to(applyee);
        if (margin)         margin->apply_to(applyee);
        if (padding)       padding->apply_to(applyee);
      }

      void format(dom::DOMString &out, StyleManipContext &ctx) const
      {
        if (background) background->format(out, ctx);
        if (margin)         margin->format(out, ctx);
        if (padding)       padding->format(out, ctx);
      }

      core::UniquePtr<BackgroundGroup> background;
      core::UniquePtr<MarginGroup>     margin;
      core::UniquePtr<PaddingGroup>    padding;
    };




    struct BoxGroup2
    {
      void apply_to(StyleApplyee &applyee) const
      {
        if (size)           size->apply_to(applyee);
        if (structure) structure->apply_to(applyee);
      }

      void format(dom::DOMString &out, StyleManipContext &ctx) const
      {
        if (size)           size->format(out, ctx);
        if (structure) structure->format(out, ctx);
      }

      core::UniquePtr<SizeGroup>      size;
      core::UniquePtr<StructureGroup> structure;
    };




    struct SpecifiedStyle
    {
      void apply_font_to(StyleApplyee &applyee) const
      {
        if (text) text->apply_font_to(applyee);
      }

      void apply_to(StyleApplyee &applyee) const
      {
        if (text)     text->apply_to(applyee);
        if (box1)     box1->apply_to(applyee);
        if (box2)     box2->apply_to(applyee);
        if (border) border->apply_to(applyee);
      }

      void format(dom::DOMString &out, StyleManipContext &ctx) const
      {
        if (text)     text->format(out, ctx);
        if (box1)     box1->format(out, ctx);
        if (box2)     box2->format(out, ctx);
        if (border) border->format(out, ctx);
      }

      core::UniquePtr<TextGroup>   text;
      core::UniquePtr<BoxGroup1>   box1;
      core::UniquePtr<BoxGroup2>   box2;
      core::UniquePtr<BorderGroup> border;
    };




    struct RareStyledNodeData: RareNodeData
    {
      ElemStyleDecl *style_decl;

      RareStyledNodeData(): style_decl(0) {}
    };




    struct ElemStyleDecl: virtual dom::css::CSSStyleDeclaration
    {
      virtual dom::DOMString getCssText() const throw ();

      virtual dom::DOMString getPropertyValue(dom::DOMString const &propertyName) const throw ();

      virtual void setProperty(dom::DOMString const &propertyName, dom::DOMString const &value,
                               dom::DOMString const &priority) throw (dom::DOMException);


      StyleManipContext &get_manip_context() const;


      bool is_referenced() const { return dom::css::CSSStyleDeclaration::is_referenced(); }

      bool is_bound() const { return elem; }

      // Declaration must be bound
      bool is_valid() const { return false; } // FIXME: Implement this!

    private:
      friend class RareNodeData;
      friend class ElemStyleDeclManager;

      static ElemStyleDecl *&subscr(RareNodeData *r) throw ()
      {
        return static_cast<RareStyledNodeData *>(r)->style_decl;
      }

      static void acquire(StyledElement *, dom::ref<ElemStyleDecl> &);

      static void release(ElemStyleDecl *, Node *) {}

      virtual void on_referenced()   const throw ();
      virtual void on_unreferenced() const throw ();

      // Must be associated with an element
      void invalidate()
      {
        // FIXME: Implement this!
      }

      StyledElement *elem;
    };




    struct StyledElement: Element, virtual dom::css::ElementCSSInlineStyle
    {
      static int const flag_pos_Valid_style_decl = Element::flag_pos_End + 0;
      static int const flag_pos_End              = Element::flag_pos_End + 1;

      static Flags const valid_style_decl = 1 << flag_pos_Valid_style_decl;

      virtual dom::ref<dom::css::CSSStyleDeclaration> getStyle() const throw ();

      StyledDocument *get_doc() const;

      SpecifiedStyle const *get_style_decl_read_ptr() const
      {
        return spec_style ? spec_style.get() : 0;
      }

      SpecifiedStyle &get_style_decl_write_ref()
      {
        if (!spec_style) spec_style.reset(new SpecifiedStyle);
        return *spec_style;
      }

      StyleManipContext &get_style_manip_context() const;

      StyledElement(StyledElemType *t);

      virtual ~StyledElement() throw ();

    protected:
      virtual void apply_default_font_to(StyleApplyee &) const {}
      virtual void apply_default_style_to(StyleApplyee &) const {}

    private:
      friend class StyleApplyee;
      friend class ElemStyleDecl;
      friend class ElemStyleDeclManager;

      core::UniquePtr<SpecifiedStyle> spec_style;
    };




    struct StyledElemType: ElemType
    {
      StyledElemType(StyledDocument *d, bool read_only, ElemKey const &k, ElemQual const &q);

      virtual StyledElement *create_element() = 0;
    };




    struct ElemStyleDeclManager
    {
      void discard_if_unref(StyledElement const *e) throw ()
      {
        ElemStyleDecl *const decl = e->get_rare_obj<ElemStyleDecl>();
        if (!decl || decl->is_referenced()) return;
        e->clear_flag(StyledElement::valid_style_decl);
        unref_queue.remove(decl);
        unref_queue.prepend(decl);
        const_cast<StyledElement *>(e)->remove_rare_obj<ElemStyleDecl>();
        decl->elem = 0;
      }

    private:
      friend class ElemStyleDecl;

      void acquire(StyledElement *e, dom::ref<ElemStyleDecl> &d)
      {
        ElemStyleDecl *decl;
        if (!unref_queue.empty()) {
          decl = unref_queue.get_first();
          if (!decl->is_bound()) goto have;
          if (min_valid_unrefs < unref_queue.size() || !decl->is_valid()) {
            const_cast<StyledElement *>(decl->elem)->remove_rare_obj<ElemStyleDecl>();
            decl->elem->clear_flag(StyledElement::valid_style_decl);
            goto have;
          }
        }
        decl = new ElemStyleDecl;
        unref_queue.prepend(decl);

      have:
        decl->elem = e;
        decl->invalidate();
        d.reset(decl);
      }

      // This queue contains any style declaration that is either not
      // bound to an element or not referenced. A declaration that is
      // bound and has a valid cache comes after any declaration that
      // is unbound or does not have a valid cache. The declarations
      // that are bound and have a valid cache are ordered according
      // to the time they became unreferenced, such that the last
      // declaration in the queue is the one that became unreferenced
      // at the latest point in time.
      SmallFixedSizeQueue<ElemStyleDecl *, 8> unref_queue;
      static int const min_valid_unrefs = 4;
    };




    struct StyledDocument: Document, StyleManipContext, StyleComputeContext
    {
      StyledDocument(StyledImplementation *i, double dpcm = get_default_dpcm());

      virtual ~StyledDocument() throw () {}

      static double get_default_dpcm()
      {
        // The calculation below is in accordance with CSS2.1.
        double ptpd  = 0.75; // Points per dot  (a dot is the same as a pixel)
        double ptpin = 72;   // Points per inch
        double cmpin = 2.54; // Centimeters per inch
        double dpcm = ptpin / cmpin / ptpd; // Dots per centimeter
        return dpcm;
      }

    private:
      friend class ElemStyleDecl;
      friend class StyledElement;
      friend class StyledImplementation;

      ElemStyleDeclManager elem_style_decl_manager;
    };




    struct StyledImplementation: DOMImplementationLS, StaticStyleInfo
    {
      virtual RareNodeData *create_rare_node_data() const
      {
        return new RareStyledNodeData;
      }

      virtual void destroy_rare_node_data(RareNodeData *r) const throw ()
      {
        delete static_cast<RareStyledNodeData *>(r);
      }

      virtual void clear_nonessential_rare_node_data(ParentNode *p) throw ()
      {
        if (StyledElement *e = dynamic_cast<StyledElement *>(p))
          e->get_doc()->elem_style_decl_manager.discard_if_unref(e);
      }

      StyledImplementation(CssLevel l): StaticStyleInfo(l) {}

      virtual ~StyledImplementation() throw () {}
    };






    template<class Group> struct PropGroupInfo;

    template<> struct PropGroupInfo<BackgroundGroup>
    {
      typedef BoxGroup1 Parent;
      static core::UniquePtr<BackgroundGroup> Parent::*get_self() { return &Parent::background; }
    };

    template<> struct PropGroupInfo<BorderGroup::Width>
    {
      typedef BorderGroup Parent;
      static BorderGroup::Width Parent::*get_self() { return &Parent::width; }
    };

    template<> struct PropGroupInfo<BorderGroup::Style>
    {
      typedef BorderGroup Parent;
      static BorderGroup::Style Parent::*get_self() { return &Parent::style; }
    };

    template<> struct PropGroupInfo<BorderGroup::Color>
    {
      typedef BorderGroup Parent;
      static BorderGroup::Color Parent::*get_self() { return &Parent::color; }
    };

    template<> struct PropGroupInfo<FontGroup>
    {
      typedef TextGroup Parent;
      static core::UniquePtr<FontGroup> Parent::*get_self() { return &Parent::font; }
    };

    template<> struct PropGroupInfo<MarginGroup>
    {
      typedef BoxGroup1 Parent;
      static core::UniquePtr<MarginGroup> Parent::*get_self() { return &Parent::margin; }
    };

    template<> struct PropGroupInfo<PaddingGroup>
    {
      typedef BoxGroup1 Parent;
      static core::UniquePtr<PaddingGroup> Parent::*get_self() { return &Parent::padding; }
    };

    template<> struct PropGroupInfo<SizeGroup>
    {
      typedef BoxGroup2 Parent;
      static core::UniquePtr<SizeGroup> Parent::*get_self() { return &Parent::size; }
    };

    template<> struct PropGroupInfo<StructureGroup>
    {
      typedef BoxGroup2 Parent;
      static core::UniquePtr<StructureGroup> Parent::*get_self() { return &Parent::structure; }
    };

    template<> struct PropGroupInfo<TextGroup>
    {
      typedef SpecifiedStyle Parent;
      static core::UniquePtr<TextGroup> Parent::*get_self() { return &Parent::text; }
    };

    template<> struct PropGroupInfo<BoxGroup1>
    {
      typedef SpecifiedStyle Parent;
      static core::UniquePtr<BoxGroup1> Parent::*get_self() { return &Parent::box1; }
    };

    template<> struct PropGroupInfo<BoxGroup2>
    {
      typedef SpecifiedStyle Parent;
      static core::UniquePtr<BoxGroup2> Parent::*get_self() { return &Parent::box2; }
    };

    template<> struct PropGroupInfo<BorderGroup>
    {
      typedef SpecifiedStyle Parent;
      static core::UniquePtr<BorderGroup> Parent::*get_self() { return &Parent::border; }
    };



    template<class Group> struct PropGroupAccess
    {
      typedef PropGroupInfo<Group> Info;
      typedef typename Info::Parent Parent;

      static Group const *get_read_ptr(StyledElement const &elem)
      {
        Parent const *const p = PropGroupAccess<Parent>::get_read_ptr(elem);
        return p ? (p->*Info::get_self()).get() : 0;
      }

      static Group &get_write_ref(StyledElement &elem)
      {
        Parent &p = PropGroupAccess<Parent>::get_write_ref(elem);
        core::UniquePtr<Group> &ptr = p.*Info::get_self();
        if (!ptr) ptr.reset(new Group);
        return *ptr;
      }
    };

    template<> struct PropGroupAccess<SpecifiedStyle>
    {
      static SpecifiedStyle const *get_read_ptr(StyledElement const &elem)
      {
        return elem.get_style_decl_read_ptr();
      }

      static SpecifiedStyle &get_write_ref(StyledElement &elem)
      {
        return elem.get_style_decl_write_ref();
      }
    };

    template<class Top, class Right, class Bottom, class Left>
    struct PropGroupAccess<RectGroup<Top, Right, Bottom, Left> >
    {
      typedef RectGroup<Top, Right, Bottom, Left> Group;
      typedef PropGroupInfo<Group> Info;
      typedef typename Info::Parent Parent;

      static Group const *get_read_ptr(StyledElement const &elem)
      {
        Parent const *const p = PropGroupAccess<Parent>::get_read_ptr(elem);
        return p ? &(p->*Info::get_self()) : 0;
      }

      static Group &get_write_ref(StyledElement &elem)
      {
        return PropGroupAccess<Parent>::get_write_ref(elem).*Info::get_self();
      }
    };



    template<class Prop, class Group> struct LonghandPropDefBase: StylePropDef
    {
      dom::DOMString get(StyledElement const &elem) const
      {
        Group const *const group = PropGroupAccess<Group>::get_read_ptr(elem);
        if (!group) return dom::DOMString();
        return elem.get_style_manip_context().format_prop(group->*prop);
      }

      LonghandPropDefBase(Prop Group::*p): prop(p) {}

    protected:
      Prop Group::*const prop;
    };


    // If \a is_narrow is true, the parse_value() method for the
    // target property is assumed to take as argument a narrow string
    // rather than a wide one. This is supposed to be used for
    // properties whose values are always confined to the portable
    // character set.
    template<class Prop, class Group, bool is_narrow = true> struct LonghandPropDef;

    template<class Prop, class Group>
    struct LonghandPropDef<Prop, Group, true>: LonghandPropDefBase<Prop, Group>
    {
      void set(dom::DOMString const &str, StyledElement &elem) const
      {
        Prop value;
        elem.get_style_manip_context().parse_narrow_prop(str, value);
        PropGroupAccess<Group>::get_write_ref(elem).*this->prop = value;
      }

      LonghandPropDef(Prop Group::*p): LonghandPropDefBase<Prop, Group>(p) {}
    };

    template<class Prop, class Group>
    struct LonghandPropDef<Prop, Group, false>: LonghandPropDefBase<Prop, Group>
    {
      void set(dom::DOMString const &str, StyledElement &elem) const
      {
        Prop value;
        elem.get_style_manip_context().parse_wide_prop(str, value);
        PropGroupAccess<Group>::get_write_ref(elem).*this->prop = value;
      }

      LonghandPropDef(Prop Group::*p): LonghandPropDefBase<Prop, Group>(p) {}
    };



    template<class Prop, class Group> StylePropDef &StaticStyleInfo::add(Prop Group::*prop)
    {
      return add(Prop::spec_type::get_name(), new LonghandPropDef<Prop, Group>(prop));
    }

    template<class ForceGroup, class Prop, class Group>
    StylePropDef &StaticStyleInfo::add2(Prop Group::*prop) { return add<Prop, ForceGroup>(prop); }











    // Implementation:

    inline StyleManipContext &ElemStyleDecl::get_manip_context() const
    {
      return elem->get_style_manip_context();
    }



    inline StyleApplyee::StyleApplyee(StyleComputeState *c):
      cont_block(0), cont_block_shrinks_to_fit(false), compute_state(*c),
      parent(0), dirty(0), accum_dirty(c->accum_dirty_bits), has_current_height_of_x(false),
      current_font_size(c->get_current_style().font.size) {}



    inline StyleApplyee::StyleApplyee(ContainingBlock *b, bool cont_block_shrinks_to_fit,
                               StyledElement *e, StyleApplyee const *p):
      cont_block(b), cont_block_shrinks_to_fit(cont_block_shrinks_to_fit),
      compute_state(p->compute_state), parent(p), dirty(0),
      accum_dirty(compute_state.accum_dirty_bits), has_current_height_of_x(false),
      current_font_size(compute_state.get_current_style().font.size)
    {
      // The 'font' group must always be applied first, such that
      // properties in the other groups can refer reliably to the
      // current font size, as well as to the current height of
      // 'x'. The 'font' group consists precisely of 'font-style',
      // 'font-variant', 'font-weight', 'font-size', and
      // 'font-family'.

      // We must first determine the font size, since other values may
      // depend on it
      e->apply_default_font_to(*this);

      // FIXME: Apply font styles from style sheets here
      if (e->spec_style) e->spec_style->apply_font_to(*this);

      // Flush the font changes such that we can meassure the height
      // of 'x', should we need it.
      ComputedStyle const &current = compute_state.get_current_style();
      if ((dirty|accum_dirty) & 1ul<<style_group_Font) {
        compute_state.change_font(current.font);
        current_font_size = current.font.size;
        has_current_height_of_x = false;
      }

      e->apply_default_style_to(*this);

      // FIXME: Apply other styles from style sheets here. The brute
      // force way is to evaluate each selector against this element,
      // and apply the style if ther is a match. This might require
      // access to ancestors, siblings, and descendants. How about
      // 'important'? Can the results be cached?
      if (e->spec_style) e->spec_style->apply_to(*this);

      bool const is_root = !e->get_parent();
      compute_state.get_current_style().constraint_fixups(is_root);

      accum_dirty |= dirty;
      compute_state.accum_dirty_bits = 0;
    }



    inline void ElemStyleDecl::acquire(StyledElement *e, dom::ref<ElemStyleDecl> &d)
    {
      e->get_doc()->elem_style_decl_manager.acquire(e,d);
    }



    inline StyledDocument *StyledElement::get_doc() const
    {
      return static_cast<StyledDocument *>(Element::get_doc());
    }



    inline StyleManipContext &StyledElement::get_style_manip_context() const
    {
      return *get_doc();
    }



    inline StyledElement::StyledElement(StyledElemType *t): Element(t), spec_style(0) {}



    inline StyledElement::~StyledElement() throw ()
    {
      // This is because when the binding between the declaration
      // object and this element is broken, the declaration object
      // needs to access this styled element while it is still a
      // StyledElement.
      get_doc()->elem_style_decl_manager.discard_if_unref(this);
    }



    inline StyledElemType::StyledElemType(StyledDocument *d, bool read_only,
                                          ElemKey const &k, ElemQual const &q):
      ElemType(d, read_only, k, q) {}


    inline StyledDocument::StyledDocument(StyledImplementation *i, double dpcm):
      Document(i), StyleManipContext(i), StyleComputeContext(dpcm) {}
  }
}

#endif // ARCHON_DOM_IMPL_STYLE_HPP
