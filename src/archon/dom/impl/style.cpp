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

#include <iostream>

#include <archon/dom/impl/style.hpp>


using namespace std;
using namespace archon::core;



namespace
{
  namespace dom = archon::dom;
  using namespace archon::dom_impl;



  template<class Prop, class Rect> struct RectShorthandPropDef: StylePropDef
  {
    typedef Prop prop_type;
    typedef Rect rect_type;

    dom::DOMString get(StyledElement const &) const
    {
      throw runtime_error("Not yet implemented"); // FIXME: Implement this!
    }

    void set(dom::DOMString const &str, StyledElement &elem) const
    {
      StyleManipContext &ctx = elem.get_style_manip_context();
      string str2;
      if (dom::str_to_narrow_port(str, str2)) {
        prop_type sides[4];
        string::const_iterator const end = str2.end();
        string::const_iterator i = str2.begin();
        int n = 0;
        for (;;) {
          for (;;) {
            if (i == end) {
              rect_type &rect = PropGroupAccess<rect_type>::get_write_ref(elem);
              switch (n) {
              case 0:
              case 1:
                rect.top = rect.right = rect.bottom = rect.left = sides[0];
                break;
              case 2:
                rect.top   = rect.bottom = sides[0];
                rect.right = rect.left   = sides[1];
                break;
              case 3:
                rect.top               = sides[0];
                rect.right = rect.left = sides[1];
                rect.bottom            = sides[2];
                break;
              case 4:
                rect.top    = sides[0];
                rect.right  = sides[1];
                rect.bottom = sides[2];
                rect.left   = sides[3];
                break;
              default:
                throw dom::DOMException(dom::SYNTAX_ERR, "Too many values");
              }
              return;
            }
            if (!StyleManipContext::is_space(*i)) break;
            ++i;
          }

          int parenth_level = 0;
          string::const_iterator j = i;
          char c = *j;
          for (;;) {
            if (c == '(') ++parenth_level;
            else if (c == ')') --parenth_level;
            if (++j == end) break;
            c = *j;
            if (parenth_level == 0 && StyleManipContext::is_space(c)) break;
          }

          string atom(i,j);
          if ((n < 4 && !sides[n].parse_value(atom, ctx))) break;
          ++n;

          i = j;
        }
      }
      throw dom::DOMException(dom::SYNTAX_ERR, "Failed to parse property value");
    }
  };



  struct BorderWidthPropDef: RectShorthandPropDef<BorderGroup::TopWidth, BorderGroup::Width> {};
  struct BorderStylePropDef: RectShorthandPropDef<BorderGroup::TopStyle, BorderGroup::Style> {};
  struct BorderColorPropDef: RectShorthandPropDef<BorderGroup::TopColor, BorderGroup::Color> {};



  template<int side> struct BorderPropDefBase: StylePropDef
  {
    dom::DOMString get(StyledElement const &) const
    {
      throw runtime_error("Not yet implemented"); // FIXME: Implement this!
    }

    void set(dom::DOMString const &str, StyledElement &elem) const
    {
      StyleManipContext &ctx = elem.get_style_manip_context();
      BorderWidthPropDef::prop_type width;
      BorderStylePropDef::prop_type style;
      BorderColorPropDef::prop_type color;
      string str2;
      if (dom::str_to_narrow_port(str, str2)) {
        string::const_iterator const end = str2.end();
        string::const_iterator i = str2.begin();
        for (;;) {
          for (;;) {
            if (i == end) {
              BorderGroup &border = PropGroupAccess<BorderGroup>::get_write_ref(elem);
              if (side == 0 || side == 4) {
                border.width.top    = width;
                border.style.top    = style;
                border.color.top    = color;
              }
              if (side == 1 || side == 4) {
                border.width.right  = width;
                border.style.right  = style;
                border.color.right  = color;
              }
              if (side == 2 || side == 4) {
                border.width.bottom = width;
                border.style.bottom = style;
                border.color.bottom = color;
              }
              if (side == 3 || side == 4) {
                border.width.left   = width;
                border.style.left   = style;
                border.color.left   = color;
              }
              return;
            }
            if (!StyleManipContext::is_space(*i)) break;
            ++i;
          }

          int parenth_level = 0;
          string::const_iterator j = i;
          char c = *j;
          for (;;) {
            if (c == '(') ++parenth_level;
            else if (c == ')') --parenth_level;
            if (++j == end) break;
            c = *j;
            if (parenth_level == 0 && StyleManipContext::is_space(c)) break;
          }

          string atom(i,j);
          if ((width || !width.parse_value(atom, ctx)) &&
              (style || !style.parse_value(atom, ctx)) &&
              (color || !color.parse_value(atom, ctx))) break;

          i = j;
        }
      }
      throw dom::DOMException(dom::SYNTAX_ERR, "Failed to parse property value");
    }
  };

  struct BorderTopPropDef:    BorderPropDefBase<0> {};
  struct BorderRightPropDef:  BorderPropDefBase<1> {};
  struct BorderBottomPropDef: BorderPropDefBase<2> {};
  struct BorderLeftPropDef:   BorderPropDefBase<3> {};

  struct BorderPropDef:       BorderPropDefBase<4> {};




  struct MarginPropDef:  RectShorthandPropDef<MarginTop,  MarginGroup> {};
  struct PaddingPropDef: RectShorthandPropDef<PaddingTop, PaddingGroup> {};
}




namespace archon
{
  namespace dom_impl
  {
    void StaticStyleInfo::add_props()
    {
      add(&BackgroundGroup::color);
      add(&BorderGroup::Width::top);
      add(&BorderGroup::Width::right);
      add(&BorderGroup::Width::bottom);
      add(&BorderGroup::Width::left);
      add(&BorderGroup::Style::top);
      add(&BorderGroup::Style::right);
      add(&BorderGroup::Style::bottom);
      add(&BorderGroup::Style::left);
      add(&BorderGroup::Color::top);
      add(&BorderGroup::Color::right);
      add(&BorderGroup::Color::bottom);
      add(&BorderGroup::Color::left);
      add("border-width",  std::make_unique<BorderWidthPropDef>());
      add("border-style",  std::make_unique<BorderStylePropDef>());
      add("border-color",  std::make_unique<BorderColorPropDef>());
      add("border-top",    std::make_unique<BorderTopPropDef>());
      add("border-right",  std::make_unique<BorderRightPropDef>());
      add("border-bottom", std::make_unique<BorderBottomPropDef>());
      add("border-left",   std::make_unique<BorderLeftPropDef>());
      add("border",        std::make_unique<BorderPropDef>());
      add(&TextGroup::color);
      add(&FontGroup::style);
      add(&FontGroup::variant);
      add(&FontGroup::weight);
      add(&FontGroup::size);
      add(&FontGroup::line_height);
      add2<MarginGroup>(&MarginGroup::top);
      add2<MarginGroup>(&MarginGroup::right);
      add2<MarginGroup>(&MarginGroup::bottom);
      add2<MarginGroup>(&MarginGroup::left);
      add("margin", std::make_unique<MarginPropDef>());
      add2<PaddingGroup>(&PaddingGroup::top);
      add2<PaddingGroup>(&PaddingGroup::right);
      add2<PaddingGroup>(&PaddingGroup::bottom);
      add2<PaddingGroup>(&PaddingGroup::left);
      add("padding", std::make_unique<PaddingPropDef>());
      add(&SizeGroup::width);
      add(&SizeGroup::height);
      add(&StructureGroup::clear);
      add(&StructureGroup::display);
      add(&StructureGroup::cssFloat);
      add(&StructureGroup::overflow);
      add(&StructureGroup::position);
    }



    void StyleManipContext::warning(dom::DOMString const &msg)
    {
      bool const fail = false;
      WideLocaleCodec codec(fail, cerr.getloc());
      cerr << "WARNING: " << codec.encode(dom::str_to_wide(msg, cerr.getloc())) << endl;
    }



    core::EnumAssoc EmptyEnumSpec::map[] =
    {
      { 0, 0 }
    };



    core::EnumAssoc LengthUnitSpec::map[] =
    {
      { lengthUnit_None,    ""   },
      { lengthUnit_Percent, "%"  },
      { lengthUnit_EM,      "em" },
      { lengthUnit_EX,      "ex" },
      { lengthUnit_PX,      "px" },
      { lengthUnit_CM,      "cm" },
      { lengthUnit_MM,      "mm" },
      { lengthUnit_IN,      "in" },
      { lengthUnit_PT,      "pt" },
      { lengthUnit_PC,      "pc" },
      { 0, 0 }
    };



    core::EnumAssoc NamedBorderWidthSpec::map[] =
    {
      { borderWidth_Thin,   "thin"   },
      { borderWidth_Medium, "medium" },
      { borderWidth_Thick,  "thick"  },
      { 0, 0 }
    };



    core::EnumAssoc BorderStyleSpec::map[] =
    {
      { borderStyle_None,   "none"   },
      { borderStyle_Hidden, "hidden" },
      { borderStyle_Dotted, "dotted" },
      { borderStyle_Dashed, "dashed" },
      { borderStyle_Solid,  "solid"  },
      { borderStyle_Double, "double" },
      { borderStyle_Groove, "groove" },
      { borderStyle_Ridge,  "ridge"  },
      { borderStyle_Inset,  "inset"  },
      { borderStyle_Outset, "outset" },
      { 0, 0 }
    };



    core::EnumAssoc FontStyleSpec::map[] =
    {
      { fontStyle_Normal,  "normal"  },
      { fontStyle_Italic,  "italic"  },
      { fontStyle_Oblique, "oblique" },
      { 0, 0 }
    };



    core::EnumAssoc FontVariantSpec::map[] =
    {
      { fontVariant_Normal,    "normal"     },
      { fontVariant_SmallCaps, "small-caps" },
      { 0, 0 }
    };



    core::EnumAssoc FontWeightSpec::map[] =
    {
      { fontWeight_100,     "100"     },
      { fontWeight_200,     "200"     },
      { fontWeight_300,     "300"     },
      { fontWeight_400,     "400"     }, // Normal
      { fontWeight_500,     "500"     },
      { fontWeight_600,     "600"     },
      { fontWeight_700,     "700"     }, // Bold
      { fontWeight_800,     "800"     },
      { fontWeight_900,     "900"     },
      { 0, 0 }
    };

    core::EnumAssoc SpecialFontWeightSpec::map[] =
    {
      { specialFontWeight_Normal,  "normal"  },
      { specialFontWeight_Bold,    "bold"    },
      { specialFontWeight_Bolder,  "bolder"  },
      { specialFontWeight_Lighter, "lighter" },
      { 0, 0 }
    };



    core::EnumAssoc NamedFontSizeSpec::map[] =
    {
      { fontSize_XXSmall, "xx-small" },
      { fontSize_XSmall,  "x-small"  },
      { fontSize_Small,   "small"    },
      { fontSize_Medium,  "medium"   },
      { fontSize_Large,   "large"    },
      { fontSize_XLarge,  "x-large"  },
      { fontSize_XXLarge, "xx-large" },
      { fontSize_Larger,  "larger"   },
      { fontSize_Smaller, "smaller"  },
      { 0, 0 }
    };



    core::EnumAssoc SystemColorSpec::map[] =
    {
      { sysColor_ActiveBorder,        "ActiveBorder"        },
      { sysColor_ActiveCaption,       "ActiveCaption"       },
      { sysColor_AppWorkspace,        "AppWorkspace"        },
      { sysColor_Background,          "Background"          },
      { sysColor_ButtonFace,          "ButtonFace"          },
      { sysColor_ButtonHighlight,     "ButtonHighlight"     },
      { sysColor_ButtonShadow,        "ButtonShadow"        },
      { sysColor_ButtonText,          "ButtonText"          },
      { sysColor_CaptionText,         "CaptionText"         },
      { sysColor_GrayText,            "GrayText"            },
      { sysColor_Highlight,           "Highlight"           },
      { sysColor_HighlightText,       "HighlightText"       },
      { sysColor_InactiveBorder,      "InactiveBorder"      },
      { sysColor_InactiveCaption,     "InactiveCaption"     },
      { sysColor_InactiveCaptionText, "InactiveCaptionText" },
      { sysColor_InfoBackground,      "InfoBackground"      },
      { sysColor_InfoText,            "InfoText"            },
      { sysColor_Menu,                "Menu"                },
      { sysColor_MenuText,            "MenuText"            },
      { sysColor_Scrollbar,           "Scrollbar"           },
      { sysColor_ThreeDDarkShadow,    "ThreeDDarkShadow"    },
      { sysColor_ThreeDFace,          "ThreeDFace"          },
      { sysColor_ThreeDHighlight,     "ThreeDHighlight"     },
      { sysColor_ThreeDLightShadow,   "ThreeDLightShadow"   },
      { sysColor_ThreeDShadow,        "ThreeDShadow"        },
      { sysColor_Window,              "Window"              },
      { sysColor_WindowFrame,         "WindowFrame"         },
      { sysColor_WindowText,          "WindowText"          },
      { 0, 0 }
    };



    core::EnumAssoc ClearValueSpec::map[] =
    {
      { clear_None,  "none"  },
      { clear_Left,  "left"  },
      { clear_Right, "right" },
      { clear_Both,  "both"  },
      { 0, 0 }
    };



    core::EnumAssoc DisplayValueSpec::map[] =
    {
      { display_Inline,           "inline"             },
      { display_Block,            "block"              },
      { display_ListItem,         "list-item"          },
      { display_InlineBlock,      "inline-block"       },
      { display_Table,            "table"              },
      { display_InlineTable,      "inline-table"       },
      { display_TableRowGroup,    "table-row-group"    },
      { display_TableHeaderGroup, "table-header-group" },
      { display_TableFooterGroup, "table-footer-group" },
      { display_TableRow,         "table-row"          },
      { display_TableColumnGroup, "table-column-group" },
      { display_TableColumn,      "table-column"       },
      { display_TableCell,        "table-cell"         },
      { display_TableCaption,     "table-caption"      },
      { display_None,             "none"               },
      { 0, 0 }
    };



    core::EnumAssoc FloatValueSpec::map[] =
    {
      { float_Left,  "left"  },
      { float_Right, "right" },
      { float_None,  "none"  },
      { 0, 0 }
    };



    core::EnumAssoc OverflowValueSpec::map[] =
    {
      { overflow_Visible, "visible" },
      { overflow_Hidden,  "hidden"  },
      { overflow_Scroll,  "scroll"  },
      { 0, 0 }
    };



    core::EnumAssoc PositionValueSpec::map[] =
    {
      { position_Static,   "static"   },
      { position_Relative, "relative" },
      { position_Absolute, "absolute" },
      { position_Fixed,    "fixed"    },
      { 0, 0 }
    };




    dom::DOMString ElemStyleDecl::getCssText() const throw ()
    {
      StyleManipContext &ctx = get_manip_context();
      dom::DOMString out;
      SpecifiedStyle *const s = elem->spec_style.get();
      if (s) s->format(out, ctx);
      return 0 < out.size() ? out.substr(0, out.size()-1) : out; // Chop off final space
    }



    dom::DOMString ElemStyleDecl::getPropertyValue(dom::DOMString const &name) const throw ()
    {
      StyleManipContext &ctx = get_manip_context();
      StylePropDef const *const prop = ctx.lookup_prop_def(name);
      return prop ? prop->get(*elem) : dom::DOMString();
    }



    void ElemStyleDecl::setProperty(dom::DOMString const &name, dom::DOMString const &value,
                                dom::DOMString const &prio) throw (dom::DOMException)
    {
      StyleManipContext &ctx = get_manip_context();
      if (ctx.parse_priority(prio) != style_priority_Normal)
        throw runtime_error("Non-default priority is not yet implemented");
      StylePropDef const *const prop = ctx.lookup_prop_def(name);
      if (prop) prop->set(value, *elem);
      else {
        ctx.unrecognized_warning(dom::str_from_port("Ignoring unrecognized property '") +
                                 name + dom::str_from_port("'"));
      }
    }



    void ElemStyleDecl::on_referenced() const throw ()
    {
      elem->bind_ref();
      ElemStyleDeclManager &manager = elem->get_doc()->elem_style_decl_manager;
      if (manager.unref_queue.get_first() == this) { // Heuristic search optimization
        manager.unref_queue.remove_first();
      }
      else {
        manager.unref_queue.remove(const_cast<ElemStyleDecl *>(this));
      }
    }



    void ElemStyleDecl::on_unreferenced() const throw ()
    {
      StyledElement const *const e = elem;
      ElemStyleDeclManager &manager = e->get_doc()->elem_style_decl_manager;
      if (is_valid()) {
        if (manager.unref_queue.full()) {
          ElemStyleDecl *const clobbered = manager.unref_queue.get_first();
          manager.unref_queue.remove_first();
          if (clobbered->is_bound()) {
            const_cast<StyledElement *>(clobbered->elem)->remove_rare_obj<ElemStyleDecl>();
            clobbered->elem->clear_flag(StyledElement::valid_style_decl);
          }
          delete clobbered;
        }
        manager.unref_queue.append(const_cast<ElemStyleDecl *>(this));
      }
      else {
        const_cast<StyledElement *>(e)->remove_rare_obj<ElemStyleDecl>();
        if (manager.unref_queue.full()) {
          delete this;
        }
        else {
          manager.unref_queue.prepend(const_cast<ElemStyleDecl *>(this));
          const_cast<ElemStyleDecl *>(this)->elem = 0;
        }
      }
      e->unbind_ref();
    }



    dom::ref<dom::css::CSSStyleDeclaration> StyledElement::getStyle() const throw ()
    {
      dom::ref<ElemStyleDecl> d;
      ensure_rare_obj(this, d);
      return d;
    }
  }
}
