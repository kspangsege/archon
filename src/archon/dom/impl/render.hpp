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

#ifndef ARCHON_DOM_IMPL_RENDER_HPP
#define ARCHON_DOM_IMPL_RENDER_HPP

#include <algorithm>




#include <iostream>





#include <archon/util/color.hpp>
#include <archon/util/packed_trgb.hpp>
#include <archon/dom/impl/style.hpp>



/*

Rendering of text (and other adjacent inline boxes):

In general, we need to know the maximum available width.

When that is known, layout the text, and that gives the height of the containing box.

Each box should optionally contain a TextLayout

 */





/*

SHRINK-TO-FIT:

PROBLEMS WITH SHRINK-TO-FIT WHEN VERTICAL PROPS OF DESCENDANT BOXES DEPEND ON AVAIL WIDTH

TRY TO COME UP WITH A TEST THAT REVEALS THE PROBLEM AND SEE HOW BROWSERS HANDLE IT

It appears that what Gecko, WebKit, and Presto does, is to consider any prop value that is relative to avail_width in first pass of shrink-to-fit, as 0, except width/height which are calculated as child_avail_width = parent_avail_width * factor.

In the second pass of shrink-to-fit the flow may change.

Therefore, the second pass of shrink-to-fit must be a general reflow_to_width() or change_width() or simply set_width() for each child. his method would also be used in general when width changes dynamically.

Height is allway shrink-to-fit.

*/

/*

Inline-boxes (background and border) are rendered in flow order, and
subordinately in nesting order (BUT, CONSIDER Z-INDEX), but all are
rendered after the border of the box that contains the inline
flow. Horizontal margin, borders, and padding contribute to box
spacing, but vertical margin, borders, and padding does not. This
applies between adjacent inline boxes as well as between an inline box
and the box that contains the inline flow. The left margin, border,
and padding is present only on the first of a series of inline boxes
generated from the same inline element. Likewise, the right padding,
border, and margin is present only on the last box. For the purpose of
word breaking, the horizontal margin, border, and padding are rigidly
attached to the non-space glyphs that they are adjacent to, that is,
the glyphs that are reachable without moving through white-space
(after white-space folding).

For all boxes (inline and block), background is rendered before
border, and border is rendered before contents (BUT, CONSIDER
Z-INDEX).

Handling inline flows:

When a text node or an inline element is seen, initialize a new text
formatter object.  Each successive text node that is part of the
initiated inline flow is submitted to the formatter. Whenever an
inline element is entered or exited, the style of the formatter needs
to be updated (how can this be done efficiently?). The text formatter
must report on generated inline boxes as they are generated. When an
inline element ends, or when the entire inline flow ends, the
formatter must be queried to understand where the current inline box
ends, and where a new one begins.


What is the meaning of `z-index:none`? See http://www.w3.org/TR/CSS2/visuren.html#propdef-z-index

Is `z-index` implicitely cast to an integer? YES


White-space processing:

normal
  white_space_folding = full; // ???
  TextFormatter::set_word_wrap_mode(TextFormatter::word_wrap_Yes);
  TextFormatter::enable_line_wrapping(false);
  TextFormatter::enable_page_wrapping(false);
pre
  white_space_folding = none;
  TextFormatter::set_word_wrap_mode(TextFormatter::word_wrap_No);
  TextFormatter::enable_line_wrapping(false);
  TextFormatter::enable_page_wrapping(false);
nowrap
pre-wrap
  white_space_folding = none;
  TextFormatter::set_word_wrap_mode(TextFormatter::word_wrap_Yes); ???
  TextFormatter::enable_line_wrapping(false);
  TextFormatter::enable_page_wrapping(false);
pre-line


*/




namespace archon {
namespace DomImpl {

struct Box;
struct RenderElement;
struct RenderDocument;


// SHOULD NON-RENDER ELEMENTS BE ALLOWED TO EXIST IN A RENDER DOCUMENT? IF NOT, ANY DERIVED CLASS MUST SOMEHOW BE FORCED TO ONLY CREATE RENDER ELEMS.




/*
struct TextManager
{
    virtual TextFormatter *new_text_formatter() = 0;
};
*/




struct Renderer {
    struct Border {
        int width;
        BorderStyle style;
        util::PackedTRGB color;
    };

    virtual void filled_box(int x, int y, int width, int height, util::PackedTRGB color) = 0;

    virtual void border_box(int x, int y, int width, int height, Border const *sides) = 0;

    virtual ~Renderer() {}
};




/// This one should be reference counted, and ther should be a hash
/// map of currently known backgrounds such that the memory usage can
/// be minimized. The same goes for all the other attachable style,
/// such as RenderBorder.
struct RenderBackground {
    util::PackedTRGB color;
};




struct RenderBorder {
    Renderer::Border sides[4];
};




struct Box: ContainingBlock {
    int get_width()  { return width;  }
    int get_height() { return height; }


    /// \param x Absolute rightward pixel position of left content
    /// edge of containing block.
    ///
    /// \param y Absolute downward pixel position of top content edge
    /// of parent box.
    void render(int x, int y, Renderer *r);

    ~Box();

private:
    friend class RenderElement;

    Box() {}

    void discard_children();

    /// \param x The rightward pixel position of the left margin edge
    /// of this box relative to the left border edge of its parent.
    ///
    /// \param avail_width The distance in pixels between the left and
    /// the right margin edges.
    ///
    /// THIS SHOULD PROBABLY BE MOVED TO RenderElement
/*
      void update(int x, int avail_width)
      {

The following constraints must hold among the used values of the other properties:

    'margin-left' + 'border-left-width' + 'padding-left' + 'width' + 'padding-right' + 'border-right-width' + 'margin-right' = width of containing block

If 'width' is not 'auto' and 'border-left-width' + 'padding-left' + 'width' + 'padding-right' + 'border-right-width' (plus any of 'margin-left' or 'margin-right' that are not 'auto') is larger than the width of the containing block, then any 'auto' values for 'margin-left' or 'margin-right' are, for the following rules, treated as zero.

If all of the above have a computed value other than 'auto', the values are said to be "over-constrained" and one of the used values will have to be different from its computed value. If the 'direction' property of the containing block has the value 'ltr', the specified value of 'margin-right' is ignored and the value is calculated so as to make the equality true. If the value of 'direction' is 'rtl', this happens to 'margin-left' instead.

If there is exactly one value specified as 'auto', its used value follows from the equality.

If 'width' is set to 'auto', any other 'auto' values become '0' and 'width' follows from the resulting equality.

If both 'margin-left' and 'margin-right' are 'auto', their used values are equal. This horizontally centers the element with respect to the edges of the containing block.
*/

        // THE FOLLOWING ASSUMES THAT RIGTH MARGIN FAILS ON OVER-CONSTRAINT - BUT THIS DEPENDS ON DIRECTION LTR/RTL

        // Make sure that width, borders, and paddings are downward clamped to zero

        // It would be nice if we did not have to recompute the width if it has not changed since the shrinked version, but that would require another way (flag) of specifying that the box width is "hard coded" or not. Shuch a flag might be helpfull anyway. (flags: flexible_width, flexible_left) and method (is_expandable() which can be queried by parent before calling)

        // Calculate left and width
/*
        if (0 <= width) {
          // If marginLeft is auto or relative to avail_width: Only calculate 'left' and no recurse on children
          return;
        }

      UPS: It is more complicated: If padding is relative to avail_width. In that case, 'width' would still be set to -1, but we should still not

        int const left_border_and_padding  = 4; // elem.style.borderLeftWidth   + elem.style.paddingLeft;
        int const right_border_and_padding = 4;
        int min_width = elem_style
        int margin_left;
          if (elem.style.width is auto) {
            margin_left  = elem.style.marginLeft  is auto ? 0 : abs(elem.style.marginLeft, avail_width);
            margin_right = elem.style.marginRight is auto ? 0 : abs(elem.style.marginRight, avail_width);
            width = avail_width - margin_left - margin_right;
            if (width < min_width) width = min_width;
          }
          else { // Relative
            width = min_width + abs_nonneg(elem.style.width, avail_width);
            if (elem.style.marginLeft is auto) {
              if (elem.style.marginRight is auto) {
                margin_left = (avail_width - width) / 2;
              }
              else {
                margin_left = avail_width - width - abs(elem.style.marginRight, avail_width);
              }
              if (margin_left < 0) margin_left = 0;
            }
            else if (elem.style.marginLeft is relative) {
              margin_left = abs(elem.style.marginLeft, avail_width);
            }
          }
          for (Box *b = first_child; b; b = b->next_sibling) {
            b->expand_to_width(int x, int avail_width);
          }
        }
        else {
          // if both margins are auto, share excess space evenly
        }
        left = x + margin_left;
*/
/*

width is set to -1 if shrink_to_fit2


      }
*/

    int left;   // Rightward pixel position of left content edge relative to left content edge of parent box.
    int top;    // Downward pixel position of top content edge relative to top content edge of parent box.

    // Among other things, these are needed so it becomes easy to get
    // at the padding edge which is needed for absolutely positioned
    // children and it may also make it easier to do the width
    // adjustments needed after shrink-to-fit.  FIXME: Consider
    // storing these in a style object like background and border.
    int pad_top, pad_right, pad_bottom, pad_left;

    core::UniquePtr<RenderBackground> background;
    core::UniquePtr<RenderBorder>     border;

    RenderElement *elem;
    Box *next_sibling;
    Box *first_child;
    ContainingBlock *parent; // Containing block


/*
    // Alpha in output is copied from c1.
    static util::PackedTRGB color_interp(double x, double x1, double x2, util::PackedTRGB c1, util::PackedTRGB c2)
    {
        math::Vec4F rgba1, rgba2;
        util::PackedTRGB::unpack(c1, rgba1);
        util::PackedTRGB::unpack(c2, rgba2);
        math::Vec3 rgb1(rgba1[0], rgba1[1], rgba1[2]);
        math::Vec3 rgb2(rgba2[0], rgba2[1], rgba2[2]);
        rgb1 = image::Color::interp(x, x1, x2, rgb1, rgb2);
        return util::PackedTRGB(float(rgb1[0]), float(rgb1[1]), float(rgb1[2]));
    }
*/
};




struct RenderDocUpdateState: StyleComputeState {
    Box *abs_pos_contaning_block;

    /// The height of the current set of collapsed top and bottom
    /// margins.
    int curr_marg;

    RenderDocUpdateState(RenderDocument *d);


    // Implementing method in StyleComputeState.
    virtual void change_font(ComputedStyle::Font const &)
    {
        // FIXME: Complete this!!!!
    }

    // Implementing method in StyleComputeState.
    virtual double determine_height_of_x()
    {
        return 10; // FIXME: Complete this!!!!
    }
};




struct RenderElemUpdateState: StyleApplyee {
    /// At entry to update_render_box(), this is the downward pixel
    /// position of the top of the set of collapsed margins that
    /// precede the new child box relative to the top content edge of
    /// the parent. If a child box is generated at all, its top margin
    /// may or may not collapse with this precedding margin.
    ///
    /// At exit, this is the position of the top of the set of
    /// collapsed margins that must precede a next in-flow child box.
    int child_y;

    /// Distance between margin edges of widest child. Only used when
    /// shrink-to-fit.
    int max_child_width;

    int final_top_marg;
    bool has_final_top_marg;

    bool bot_marg_clear;

    // For the root
    RenderElemUpdateState(RenderDocUpdateState *d);

    RenderElemUpdateState(ContainingBlock *cont_block, bool cont_block_shrinks_to_fit,
                          RenderElement *e, RenderElemUpdateState *p);
};




struct RenderElement: StyledElement {
    RenderElement(StyledElemType *t): StyledElement(t), render_box(0) {}

    virtual ~RenderElement() throw ()
    {
        if (render_box)
            render_box->elem = 0;
    }

private:
    friend class Box;
    friend class RenderDocument;

    static Box *update_render_box(Element *e, ContainingBlock *c, bool cont_block_shrinks_to_fit,
                                  RenderElemUpdateState &parent_elem_st)
    {
        if (RenderElement *r = dynamic_cast<RenderElement *>(e))
            return r->update_render_box(c, cont_block_shrinks_to_fit, parent_elem_st);

        // FIXME: Handle non-render elements by assuming they have
        // default style. The generated boxes will be anonymous.
        throw std::runtime_error("Not implemented");
    }




    // FIXME: When in SHRINK-TO-FIT mode, do not process children of
    // elements with fixed width or a max-width that is less than the
    // current maximum. What about dynamic appearance of scrollbars?
    // It may sometimes be needed to know the height of bloks with
    // fixed width. Also, do not consider process absolutely
    // positioned elements.


    Box* update_render_box(ContainingBlock* cont_block, bool cont_block_shrinks_to_fit,
                           RenderElemUpdateState& parent_elem_st)
    {
std::string sssss;
dom::str_to_narrow_port(get_type()->key.tag_name, sssss);
std::cerr << "ELEM: " << sssss << std::endl;

        if (!render_box) { // or if it is not up-to-date
            RenderElemUpdateState elem_st(cont_block, cont_block_shrinks_to_fit,
                                          this, &parent_elem_st);

            DisplayValue disp = elem_st.get<PropSpec_Display>();
            if (disp == display_None)
                return nullptr;

            core::UniquePtr<Box> box(new Box);
            box->elem = this;
            box->first_child = nullptr;
            box->parent = cont_block;

            PositionValue pos = elem_st.get<PropSpec_Position>();
            bool is_abs_pos = pos == position_Absolute || pos == position_Fixed;
            FloatValue css_float = is_abs_pos ? float_None : elem_st.get<PropSpec_Float>();
            bool is_float = css_float != float_None;
            bool is_root = !get_parent();
            bool out_of_flow = is_root || is_abs_pos || is_float;
/*
            bool const is_block_level_box =
                disp == display_Block || disp == display_ListItem || disp == display_Table;
*/
            bool is_block_container_box =
                disp == display_Block || disp == display_InlineBlock || disp == display_ListItem ||
                disp == display_TableCell || disp == display_TableCaption;
            OverflowValue overflow = elem_st.get<PropSpec_Overflow>();
            bool is_root_of_block_formatting_context =
                is_abs_pos || is_float || (is_block_container_box && disp != display_Block) ||
                disp == display_InlineBlock || (disp == display_Block && overflow != overflow_Visible);

            box->pad_top    = elem_st.get<PropSpec_PaddingTop>();
            box->pad_right  = elem_st.get<PropSpec_PaddingRight>();
            box->pad_bottom = elem_st.get<PropSpec_PaddingBottom>();
            box->pad_left   = elem_st.get<PropSpec_PaddingLeft>();

            RenderDocUpdateState& doc_st =
                static_cast<RenderDocUpdateState&>(parent_elem_st.get_compute_state());

            ClearValue clear = elem_st.get<PropSpec_Clear>();

            // COLLAPSING MARGINS: Consider negative margins - special
            // rules for collapsing negative margins (keep separate
            // the maximum positive margin and the minimum negative
            // margin, both are zero initially, then the final margin
            // is max pos marg + min neg marg)

            // Handle top margin
            {
                int top_marg = elem_st.get<PropSpec_MarginTop>();
                if (out_of_flow || clear != clear_None) {
                    if (!parent_elem_st.has_final_top_marg) {
                        parent_elem_st.final_top_marg = doc_st.curr_marg;
                        parent_elem_st.has_final_top_marg = true;
                    }
                    parent_elem_st.child_y += doc_st.curr_marg;
                    doc_st.curr_marg = top_marg;
                }
                else { // Collapses with preceding margin
                    if (doc_st.curr_marg < top_marg)
                        doc_st.curr_marg = top_marg;
                }
            }

            int top_border_padding = elem_st.get<PropSpec_BorderTopWidth>() + box->pad_top;
            int left_right;
            bool have_left_right = false;

            // Assign tentative values for 'left', 'width', 'top', and
            // 'height' NOTE: Consider storing, with the computed
            // style, a flag that tells us whether the margin edge
            // width is equal to the content width. This may simply be
            // a null pointer.
            box->left = elem_st.get<PropSpec_MarginLeft>() +
                elem_st.get<PropSpec_BorderLeftWidth>() + box->pad_left;
            box->width = elem_st.get<PropSpec_Width>(); // -1 if auto
            if (box->width < 0) {
                left_right = box->left + elem_st.get<PropSpec_MarginRight>() +
                    elem_st.get<PropSpec_BorderRightWidth>() + box->pad_right;
                have_left_right = true;
                box->width = cont_block->width - left_right;
            }
            box->top = parent_elem_st.child_y + top_border_padding; // Lacks top margin
            // -1 if auto, or if relative and height of containing block is unknown
            box->height = elem_st.get<PropSpec_Height>();

            bool is_abs_width = elem_st.is_abs_comp_len<PropSpec_Width>();
            bool shrink_to_fit_initiator =
                !is_abs_width && (is_abs_pos || is_float /*|| is_table_cell_in_some_cases*/);
            bool shrink_to_fit =
                shrink_to_fit_initiator || !is_abs_width && cont_block_shrinks_to_fit;

            // Note: WebKit fails by allowing top and bottom margins
            // of an empty block level element with
            // style.overflow!='visible' to collapse. This is against
            // the specification because such an element is the root
            // of a block formatting context.
            int curr_marg_backup;
            if (top_border_padding || is_root_of_block_formatting_context) {
                elem_st.final_top_marg = doc_st.curr_marg;
                elem_st.has_final_top_marg = true;
                doc_st.curr_marg = 0;
            }
            else {
                // Only used when shrink-to-fit is initiated
                curr_marg_backup = doc_st.curr_marg;
            }

            // Loop over children
            bool has_inflow_children = false;
          children_again:
            {
                Box* prev_box = nullptr;
                for (Node* c = get_first_child(); c; c = c->get_next_sibling()) {
                    if (c->get_type()->id == dom::Node::TEXT_NODE) {
                        ARCHON_ASSERT_1(dynamic_cast<Text*>(c), "Bad text node");
                        Text* child = static_cast<Text*>(c);
                        std::string s_xx;
                        dom::str_to_narrow_port(child->get_data(), s_xx);
                        std::cerr << "TEXT("<<s_xx<<")!!!\n";
                        // FIXME: Should this set has_inflow_children to true?
                    }
                    else if (c->get_type()->id == dom::Node::ELEMENT_NODE) {
                        ARCHON_ASSERT_1(dynamic_cast<Element*>(c), "Bad element");
                        Element* child = static_cast<Element*>(c);
                        // NOTE: Absolutely positioned elements care not about
                        // the content edge of the containing block, but the
                        // padding edge. This applies to position and size only
                        // (not true, this is the way it is in WebKit, but all
                        // the other browsers, and the standard disagree, that
                        // is, the padding edge that all relative values refer
                        // to).
                        Box* child_box = update_render_box(child, box.get(), shrink_to_fit, elem_st);
                        if (!child_box)
                            continue;
                        if (prev_box) {
                            prev_box->next_sibling = child_box;
                        }
                        else {
                            box->first_child = child_box;
                        }
                        prev_box = child_box;
                    }
                }
                if (prev_box) {
                    prev_box->next_sibling = nullptr;
                    has_inflow_children = true;
                }
            }

            // Update 'width'
            if (shrink_to_fit) {
                box->width = elem_st.max_child_width;
                if (shrink_to_fit_initiator) {
                    elem_st.child_y = 0;
                    if (top_border_padding || is_root_of_block_formatting_context) {
                        doc_st.curr_marg = 0;
                    }
                    else {
                        elem_st.has_final_top_marg = false;
                        doc_st.curr_marg = curr_marg_backup;
                    }
                    elem_st.bot_marg_clear = false;

                    // Temporary fix
                    box->discard_children();
                    shrink_to_fit = false;
                    goto children_again;
//              for (Box *b = box->first_child; b; b = b->next_sibling)
//                /* b->update(cont_block_shrinks_to_fit = false) */;
                }
            }

            // Update 'left'
            if (!cont_block_shrinks_to_fit) {
                if (elem_st.is_auto_comp_len<PropSpec_MarginLeft>()) {
                    if (!have_left_right) {
                        left_right = box->left + elem_st.get<PropSpec_MarginRight>() +
                            elem_st.get<PropSpec_BorderRightWidth>() + box->pad_right;
                        have_left_right = true;
                    }
                    int excess = cont_block->width - box->width - left_right;
                    box->left = std::max(elem_st.is_auto_comp_len<PropSpec_MarginRight>() ?
                                         excess/2 : excess, 0);
                }
            }

            // Update parents maximum child width
            if (cont_block_shrinks_to_fit) {
                if (!have_left_right) {
                    left_right = box->left + elem_st.get<PropSpec_MarginRight>() +
                        elem_st.get<PropSpec_BorderRightWidth>() + box->pad_right;
                }
                int w = box->width + left_right;
                if (parent_elem_st.max_child_width < w)
                    parent_elem_st.max_child_width = w;
            }

            // Handle bottom margin
            bool bot_marg_collapses = false;
            int bot_border_padding =
                elem_st.get<PropSpec_BorderBottomWidth>() + box->pad_bottom;
            if (!bot_border_padding && !is_root_of_block_formatting_context) {
                if (!elem_st.has_final_top_marg) {
                    if (box->height <= 0 && !has_inflow_children /* && elem_st.get<PropSpec_MinHeight>() == 0 */) {
                        // Bottom margin collapses with top margin
                        if (clear != clear_None)
                            parent_elem_st.bot_marg_clear = true;
                        bot_marg_collapses = true;
                    }
                }
                else {
                    if (box->height < 0 && !elem_st.bot_marg_clear) {
                        // Bottom margin collapses with bottom margin of child
                        parent_elem_st.bot_marg_clear = false;
                        bot_marg_collapses = true;
                    }
                }
            }
            {
                int bot_marg = elem_st.get<PropSpec_MarginBottom>();
                if (bot_marg_collapses) {
                    if (doc_st.curr_marg < bot_marg)
                        doc_st.curr_marg = bot_marg;
                }
                else {
                    if (!elem_st.has_final_top_marg) {
                        elem_st.final_top_marg = doc_st.curr_marg;
                        elem_st.has_final_top_marg = true;
                    }
                    elem_st.child_y += doc_st.curr_marg;
                    doc_st.curr_marg = bot_marg;
                }
            }

            // Update top'
            box->top += elem_st.has_final_top_marg ? elem_st.final_top_marg : doc_st.curr_marg;

            // Update 'height'
            if (box->height < 0)
                box->height = elem_st.child_y;

            // Advance Y-position for next child of parent
            if (elem_st.has_final_top_marg)
                parent_elem_st.child_y = box->top + box->height + bot_border_padding;

            // WEIRD: It seems like Gecko and Presto put the outline
            // around the total height of the contents disregarding
            // any absolute height specification on the element that
            // is lower. Is this according to spec?

            if (!parent_elem_st.has_final_top_marg && elem_st.has_final_top_marg) {
                parent_elem_st.final_top_marg = elem_st.final_top_marg;
                parent_elem_st.has_final_top_marg = true;
            }

            if (elem_st.has(style_group_Background)) {
                core::UniquePtr<RenderBackground> b(new RenderBackground);
                b->color = elem_st.get<PropSpec_BackgroundColor>();
                box->background.reset(b.release());
            }

            if (elem_st.has(style_group_Border)) {
                core::UniquePtr<RenderBorder> b(new RenderBorder);
                b->sides[0].width = elem_st.get<PropSpec_BorderTopWidth>();
                b->sides[0].style = elem_st.get<PropSpec_BorderTopStyle>();
                b->sides[0].color = elem_st.get<PropSpec_BorderTopColor>();
                b->sides[1].width = elem_st.get<PropSpec_BorderRightWidth>();
                b->sides[1].style = elem_st.get<PropSpec_BorderRightStyle>();
                b->sides[1].color = elem_st.get<PropSpec_BorderRightColor>();
                b->sides[2].width = elem_st.get<PropSpec_BorderBottomWidth>();
                b->sides[2].style = elem_st.get<PropSpec_BorderBottomStyle>();
                b->sides[2].color = elem_st.get<PropSpec_BorderBottomColor>();
                b->sides[3].width = elem_st.get<PropSpec_BorderLeftWidth>();
                b->sides[3].style = elem_st.get<PropSpec_BorderLeftStyle>();
                b->sides[3].color = elem_st.get<PropSpec_BorderLeftColor>();
                box->border.reset(b.release());
            }

            render_box = box.release();
        }
        return render_box;
    }


    Box* render_box;
};



struct RenderDocument: StyledDocument {
    void update_render_tree(int avail_width, int avail_height, bool shrink_to_fit)
    {
        if (root_box)
            return;
        Element *const root = get_root();
        ContainingBlock init_cont_block;
        init_cont_block.width  = avail_width;
        init_cont_block.height = avail_height;
        RenderDocUpdateState doc_st(this);
        RenderElemUpdateState elem_st(&doc_st);
        if (shrink_to_fit) {
            delete RenderElement::update_render_box(root, &init_cont_block, true, elem_st);
            init_cont_block.width = elem_st.max_child_width;
            elem_st.child_y = 0;
            doc_st.curr_marg = 0;
            elem_st.bot_marg_clear = false;
        }
        Box *const b = RenderElement::update_render_box(root, &init_cont_block, false, elem_st);
        root_box.reset(b);
        root_box_width  = init_cont_block.width;
        root_box_height = elem_st.child_y + doc_st.curr_marg;
    }

    // Returns pixel distance between left and right margin edges of root box.
    int get_root_box_width() const { return root_box_width; }

    // Returns pixel distance between top and bottom margin edges of root box.
    int get_root_box_height() const { return root_box_height; }

    void render(Renderer *r, int x=0, int y=0)
    {
        if (root_box)
            root_box->render(x,y,r);
    }

    RenderDocument(StyledImplementation *i): StyledDocument(i) {}

    virtual ~RenderDocument() throw () {}

private:
    core::UniquePtr<Box> root_box;
    int root_box_width;
    int root_box_height;
};








// Implementation :

inline void Box::render(int x, int y, Renderer *renderer)
{
    x += left;
    y += top;

    if (background || border) {
        int t = pad_top, r = pad_right, b = pad_bottom, l = pad_left;
        if (border) {
            t += border->sides[0].width;
            r += border->sides[1].width;
            b += border->sides[2].width;
            l += border->sides[3].width;
        }

        int x0 = x-l, y0 = y-t, w = l+width+r, h = t+height+b;
        if (background)
            renderer->filled_box(x0, y0, w, h, background->color);

        if (border)
            renderer->border_box(x0, y0, w, h, border->sides);

/*
        int const w0 = border->sides[0].width;
        BorderStyle const s0 = border->sides[0].style;
        if (border->sides[1].style == s0 &&
            border->sides[2].style == s0 &&
            border->sides[3].style == s0 &&
            border->sides[1].width == w0 &&
            border->sides[2].width == w0 &&
            border->sides[3].width == w0) {
            util::PackedTRGB c[4] = {
                border->sides[0].color,
                border->sides[1].color,
                border->sides[2].color,
                border->sides[3].color
            };
            switch (s0) {

                case borderStyle_None:
                case borderStyle_Hidden:
                    break;

                case borderStyle_Dotted:
                    renderer->border_box(x0, y0, w, h, w0, Renderer::border_style_Dotted, c);
                    break;

                case borderStyle_Dashed:
                    renderer->border_box(x0, y0, w, h, w0, Renderer::border_style_Dashed, c);
                    break;

                case borderStyle_Double:
                    if (3 <= w0) {
                        int w2 = w0 / 3;
                        int w3 = (w0 - w2)/2;
                        int w1 = w0 - w2 - w3;
                        renderer->border_box(x0, y0, w, h, w1, Renderer::border_style_Solid, c);
                        int w12 = w1 + w2;
                        x0 += w12;
                        y0 += w12;
                        w -= 2*w12;
                        h -= 2*w12;
                        renderer->border_box(x0, y0, w, h, w3, Renderer::border_style_Solid, c);
                        break;
                    }
                case borderStyle_Solid:
                    renderer->border_box(x0, y0, w, h, w0, Renderer::border_style_Solid, c);
                    break;

                case borderStyle_Groove:
                    if (2 <= w0) {
                    }
                case borderStyle_Inset:
                    throw std::runtime_error("Not yet implemented");

                case borderStyle_Ridge:
                    if (2 <= w0) {
                        throw std::runtime_error("Not yet implemented");
                    }
                case borderStyle_Outset:
                    // Gecko: Highlighted sides (right + bottom) are
                    // blended with 57% full white. Shadowed sides
                    // (top + left) are blended with 68% full black.
                    util::PackedTRGB c2[4] = {
                        color_interp(68, 0, 100, util::PackedTRGB::black, c[0]),
                        color_interp(57, 0, 100, util::PackedTRGB::white, c[1]),
                        color_interp(57, 0, 100, util::PackedTRGB::white, c[2]),
                        color_interp(68, 0, 100, util::PackedTRGB::black, c[3])
                    };
                    renderer->border_box(x0, y0, w, h, w0, Renderer::border_style_Solid, c2);
            }
        }
        else {
            for (int i=0; i<4; ++i) {
                RenderBorder::Side const &side = border->sides[i];
                int x0 = x, y0 = y;
                if (i == 1 || i == 2)
                    x0 += w;
                if (i == 2 || i == 3)
                    y0 += h;
                int l = i == 0 || i == 2 ? w : h;
                int b0 = side.width;
                int b1 = border->sides[(i+3)%4].width;
                int b2 = border->sides[(i+1)%4].width;
                switch (side.style) {
                    case borderStyle_None:
                    case borderStyle_Hidden:
                        break;

                    case borderStyle_Dotted:
                        renderer->border_side(i, x0, y0, l, b0, b1, b2,
                                              Renderer::border_style_Dotted, side.color); break;

                    case borderStyle_Dashed:
                        renderer->border_side(i, x0, y0, l, b0, b1, b2,
                                              Renderer::border_style_Dashed, side.color); break;

                    case borderStyle_Double:
                        if (3 <= b) {
                            int b2 = b / 3;
                            int b4 = (b - b2)/2;
                            int b1 = b - b2 - b3;
                            renderer->border_side(i, x0, y0, l, b1, w1, Renderer::border_style_Solid, c);
                            int w12 = w1 + w2;
                            x0 += w12;
                            y0 += w12;
                            w -= 2*w12;
                            h -= 2*w12;
                            renderer->border_box(x0, y0, w, h, w3, Renderer::border_style_Solid, c);
                            break;
                        }
                    case borderStyle_Solid:
                        renderer->border_side(i, x0, y0, l, side.width, b0, b1,
                                              Renderer::border_style_Solid, side.color); break;

                    case borderStyle_Groove:
                    case borderStyle_Ridge:
                    case borderStyle_Inset:
                    case borderStyle_Outset:
                        throw std::runtime_error("Not yet implemented");
                }
            }
        }
    }
*/
    }

    for (Box *b = first_child; b; b = b->next_sibling)
        b->render(x, y, renderer);
}


inline Box::~Box()
{
    discard_children();
    if (elem)
        elem->render_box = 0;
}



inline void Box::discard_children()
{
    Box *b = first_child;
    while (b) {
        Box *const c = b->next_sibling;
        delete b;
        b = c;
    }
    first_child = 0;
}



inline RenderDocUpdateState::RenderDocUpdateState(RenderDocument *d):
    StyleComputeState(d), curr_marg(0)
{
}



inline RenderElemUpdateState::RenderElemUpdateState(RenderDocUpdateState *d):
    StyleApplyee(d), child_y(0), max_child_width(0),
    has_final_top_marg(false), bot_marg_clear(false)
{
}



inline RenderElemUpdateState::RenderElemUpdateState(ContainingBlock *b,
                                                    bool cont_block_shrinks_to_fit,
                                                    RenderElement *e,
                                                    RenderElemUpdateState *p):
    StyleApplyee(b, cont_block_shrinks_to_fit, e, p), child_y(0), max_child_width(0),
    has_final_top_marg(false), bot_marg_clear(false)
{
}


} // namespace DomImpl
} // namespace archon

#endif // ARCHON_DOM_IMPL_RENDER_HPP
