import enum
import collections
import html.parser
import archon.dom


# The HTML standard: https://html.spec.whatwg.org
#
# REFERENCE SNAPSHOT: https://html.spec.whatwg.org/commit-snapshots/00b42be693bf53ef2990ccb4f4da9df22d1b3df8/
#


class Parser:
    def __init__(self):
        self._element_names_basis = _StringRegistry.Basis()
        self._initialize()

    # `source` must be an instance of `Source`
    # `callbacks` must be an instance of `Callbacks`
    # `error_handler` must be an instance of `ErrorHandler`
    # `logger` must be an instance of `archon.log.Logger`
    def parse(self, source, document, callbacks, error_handler, logger):
        session = _Session(self, source, document, callbacks, error_handler, logger)
        session.run()

    def _initialize(self):
        self._elem_name_html           = self._element_names_basis.register("html")
        self._elem_name_head           = self._element_names_basis.register("head")
        self._elem_name_title          = self._element_names_basis.register("title")
        self._elem_name_base           = self._element_names_basis.register("base")
        self._elem_name_link           = self._element_names_basis.register("link")
        self._elem_name_meta           = self._element_names_basis.register("meta")
        self._elem_name_style          = self._element_names_basis.register("style")
        self._elem_name_body           = self._element_names_basis.register("body")
        self._elem_name_article        = self._element_names_basis.register("article")
        self._elem_name_section        = self._element_names_basis.register("section")
        self._elem_name_nav            = self._element_names_basis.register("nav")
        self._elem_name_aside          = self._element_names_basis.register("aside")
        self._elem_name_h1             = self._element_names_basis.register("h1")
        self._elem_name_h2             = self._element_names_basis.register("h2")
        self._elem_name_h3             = self._element_names_basis.register("h3")
        self._elem_name_h4             = self._element_names_basis.register("h4")
        self._elem_name_h5             = self._element_names_basis.register("h5")
        self._elem_name_h6             = self._element_names_basis.register("h6")
        self._elem_name_hgroup         = self._element_names_basis.register("hgroup")
        self._elem_name_header         = self._element_names_basis.register("header")
        self._elem_name_footer         = self._element_names_basis.register("footer")
        self._elem_name_address        = self._element_names_basis.register("address")
        self._elem_name_p              = self._element_names_basis.register("p")
        self._elem_name_hr             = self._element_names_basis.register("hr")
        self._elem_name_pre            = self._element_names_basis.register("pre")
        self._elem_name_blockquote     = self._element_names_basis.register("blockquote")
        self._elem_name_ol             = self._element_names_basis.register("ol")
        self._elem_name_ul             = self._element_names_basis.register("ul")
        self._elem_name_menu           = self._element_names_basis.register("menu")
        self._elem_name_li             = self._element_names_basis.register("li")
        self._elem_name_dl             = self._element_names_basis.register("dl")
        self._elem_name_dt             = self._element_names_basis.register("dt")
        self._elem_name_dd             = self._element_names_basis.register("dd")
        self._elem_name_figure         = self._element_names_basis.register("figure")
        self._elem_name_figcaption     = self._element_names_basis.register("figcaption")
        self._elem_name_main           = self._element_names_basis.register("main")
        self._elem_name_search         = self._element_names_basis.register("search")
        self._elem_name_div            = self._element_names_basis.register("div")
        self._elem_name_a              = self._element_names_basis.register("a")
        self._elem_name_em             = self._element_names_basis.register("em")
        self._elem_name_strong         = self._element_names_basis.register("strong")
        self._elem_name_small          = self._element_names_basis.register("small")
        self._elem_name_s              = self._element_names_basis.register("s")
        self._elem_name_ruby           = self._element_names_basis.register("ruby")
        self._elem_name_rt             = self._element_names_basis.register("rt")
        self._elem_name_rp             = self._element_names_basis.register("rp")
        self._elem_name_code           = self._element_names_basis.register("code")
        self._elem_name_var            = self._element_names_basis.register("var")
        self._elem_name_sub            = self._element_names_basis.register("sub")
        self._elem_name_sup            = self._element_names_basis.register("sup")
        self._elem_name_i              = self._element_names_basis.register("i")
        self._elem_name_b              = self._element_names_basis.register("b")
        self._elem_name_u              = self._element_names_basis.register("u")
        self._elem_name_span           = self._element_names_basis.register("span")
        self._elem_name_br             = self._element_names_basis.register("br")
        self._elem_name_wbr            = self._element_names_basis.register("wbr")
        self._elem_name_source         = self._element_names_basis.register("source")
        self._elem_name_img            = self._element_names_basis.register("img")
        self._elem_name_image          = self._element_names_basis.register("image") # Not a standard HTML element
        self._elem_name_iframe         = self._element_names_basis.register("iframe")
        self._elem_name_embed          = self._element_names_basis.register("embed")
        self._elem_name_object         = self._element_names_basis.register("object")
        self._elem_name_track          = self._element_names_basis.register("track")
        self._elem_name_area           = self._element_names_basis.register("area")
        self._elem_name_table          = self._element_names_basis.register("table")
        self._elem_name_caption        = self._element_names_basis.register("caption")
        self._elem_name_colgroup       = self._element_names_basis.register("colgroup")
        self._elem_name_col            = self._element_names_basis.register("col")
        self._elem_name_thead          = self._element_names_basis.register("thead")
        self._elem_name_tbody          = self._element_names_basis.register("tbody")
        self._elem_name_tfoot          = self._element_names_basis.register("tfoot")
        self._elem_name_tr             = self._element_names_basis.register("tr")
        self._elem_name_td             = self._element_names_basis.register("td")
        self._elem_name_th             = self._element_names_basis.register("th")
        self._elem_name_form           = self._element_names_basis.register("form")
        self._elem_name_input          = self._element_names_basis.register("input")
        self._elem_name_button         = self._element_names_basis.register("button")
        self._elem_name_select         = self._element_names_basis.register("select")
        self._elem_name_optgroup       = self._element_names_basis.register("optgroup")
        self._elem_name_option         = self._element_names_basis.register("option")
        self._elem_name_textarea       = self._element_names_basis.register("textarea")
        self._elem_name_fieldset       = self._element_names_basis.register("fieldset")
        self._elem_name_details        = self._element_names_basis.register("details")
        self._elem_name_summary        = self._element_names_basis.register("summary")
        self._elem_name_dialog         = self._element_names_basis.register("dialog")
        self._elem_name_script         = self._element_names_basis.register("script")
        self._elem_name_noscript       = self._element_names_basis.register("noscript")
        self._elem_name_template       = self._element_names_basis.register("template")
        self._elem_name_applet         = self._element_names_basis.register("applet")
        self._elem_name_bgsound        = self._element_names_basis.register("bgsound")
        self._elem_name_dir            = self._element_names_basis.register("dir")
        self._elem_name_frame          = self._element_names_basis.register("frame")
        self._elem_name_frameset       = self._element_names_basis.register("frameset")
        self._elem_name_noframes       = self._element_names_basis.register("noframes")
        self._elem_name_keygen         = self._element_names_basis.register("keygen")
        self._elem_name_listing        = self._element_names_basis.register("listing")
        self._elem_name_noembed        = self._element_names_basis.register("noembed")
        self._elem_name_param          = self._element_names_basis.register("param")
        self._elem_name_plaintext      = self._element_names_basis.register("plaintext")
        self._elem_name_rb             = self._element_names_basis.register("rb")
        self._elem_name_rtc            = self._element_names_basis.register("rtc")
        self._elem_name_strike         = self._element_names_basis.register("strike")
        self._elem_name_xmp            = self._element_names_basis.register("xmp")
        self._elem_name_basefont       = self._element_names_basis.register("basefont")
        self._elem_name_big            = self._element_names_basis.register("big")
        self._elem_name_center         = self._element_names_basis.register("center")
        self._elem_name_font           = self._element_names_basis.register("font")
        self._elem_name_marquee        = self._element_names_basis.register("marquee")
        self._elem_name_nobr           = self._element_names_basis.register("nobr")
        self._elem_name_tt             = self._element_names_basis.register("tt")

        self._elem_name_math           = self._element_names_basis.register("math")
        self._elem_name_mglyph         = self._element_names_basis.register("mglyph")
        self._elem_name_mi             = self._element_names_basis.register("mi")
        self._elem_name_mn             = self._element_names_basis.register("mn")
        self._elem_name_mo             = self._element_names_basis.register("mo")
        self._elem_name_mtext          = self._element_names_basis.register("mtext")
        self._elem_name_ms             = self._element_names_basis.register("ms")
        self._elem_name_malignmark     = self._element_names_basis.register("malignmark")
        self._elem_name_annotation_xml = self._element_names_basis.register("annotation-xml")

        self._elem_name_svg            = self._element_names_basis.register("svg")
        self._elem_name_desc           = self._element_names_basis.register("desc")
        self._elem_name_foreign_object = self._element_names_basis.register("foreignObject")

        self._html_elem_html             = (Namespace.HTML, self._elem_name_html)
        self._html_elem_head             = (Namespace.HTML, self._elem_name_head)
        self._html_elem_title            = (Namespace.HTML, self._elem_name_title)
        self._html_elem_base             = (Namespace.HTML, self._elem_name_base)
        self._html_elem_link             = (Namespace.HTML, self._elem_name_link)
        self._html_elem_meta             = (Namespace.HTML, self._elem_name_meta)
        self._html_elem_style            = (Namespace.HTML, self._elem_name_style)
        self._html_elem_body             = (Namespace.HTML, self._elem_name_body)
        self._html_elem_article          = (Namespace.HTML, self._elem_name_article)
        self._html_elem_section          = (Namespace.HTML, self._elem_name_section)
        self._html_elem_nav              = (Namespace.HTML, self._elem_name_nav)
        self._html_elem_aside            = (Namespace.HTML, self._elem_name_aside)
        self._html_elem_h1               = (Namespace.HTML, self._elem_name_h1)
        self._html_elem_h2               = (Namespace.HTML, self._elem_name_h2)
        self._html_elem_h3               = (Namespace.HTML, self._elem_name_h3)
        self._html_elem_h4               = (Namespace.HTML, self._elem_name_h4)
        self._html_elem_h5               = (Namespace.HTML, self._elem_name_h5)
        self._html_elem_h6               = (Namespace.HTML, self._elem_name_h6)
        self._html_elem_hgroup           = (Namespace.HTML, self._elem_name_hgroup)
        self._html_elem_header           = (Namespace.HTML, self._elem_name_header)
        self._html_elem_footer           = (Namespace.HTML, self._elem_name_footer)
        self._html_elem_address          = (Namespace.HTML, self._elem_name_address)
        self._html_elem_p                = (Namespace.HTML, self._elem_name_p)
        self._html_elem_hr               = (Namespace.HTML, self._elem_name_hr)
        self._html_elem_pre              = (Namespace.HTML, self._elem_name_pre)
        self._html_elem_blockquote       = (Namespace.HTML, self._elem_name_blockquote)
        self._html_elem_ol               = (Namespace.HTML, self._elem_name_ol)
        self._html_elem_ul               = (Namespace.HTML, self._elem_name_ul)
        self._html_elem_menu             = (Namespace.HTML, self._elem_name_menu)
        self._html_elem_li               = (Namespace.HTML, self._elem_name_li)
        self._html_elem_dl               = (Namespace.HTML, self._elem_name_dl)
        self._html_elem_dt               = (Namespace.HTML, self._elem_name_dt)
        self._html_elem_dd               = (Namespace.HTML, self._elem_name_dd)
        self._html_elem_figure           = (Namespace.HTML, self._elem_name_figure)
        self._html_elem_figcaption       = (Namespace.HTML, self._elem_name_figcaption)
        self._html_elem_main             = (Namespace.HTML, self._elem_name_main)
        self._html_elem_search           = (Namespace.HTML, self._elem_name_search)
        self._html_elem_div              = (Namespace.HTML, self._elem_name_div)
        self._html_elem_a                = (Namespace.HTML, self._elem_name_a)
        self._html_elem_rt               = (Namespace.HTML, self._elem_name_rt)
        self._html_elem_rp               = (Namespace.HTML, self._elem_name_rp)
        self._html_elem_br               = (Namespace.HTML, self._elem_name_br)
        self._html_elem_wbr              = (Namespace.HTML, self._elem_name_wbr)
        self._html_elem_source           = (Namespace.HTML, self._elem_name_source)
        self._html_elem_img              = (Namespace.HTML, self._elem_name_img)
        self._html_elem_iframe           = (Namespace.HTML, self._elem_name_iframe)
        self._html_elem_embed            = (Namespace.HTML, self._elem_name_embed)
        self._html_elem_object           = (Namespace.HTML, self._elem_name_object)
        self._html_elem_track            = (Namespace.HTML, self._elem_name_track)
        self._html_elem_area             = (Namespace.HTML, self._elem_name_area)
        self._html_elem_table            = (Namespace.HTML, self._elem_name_table)
        self._html_elem_caption          = (Namespace.HTML, self._elem_name_caption)
        self._html_elem_colgroup         = (Namespace.HTML, self._elem_name_colgroup)
        self._html_elem_col              = (Namespace.HTML, self._elem_name_col)
        self._html_elem_thead            = (Namespace.HTML, self._elem_name_thead)
        self._html_elem_tbody            = (Namespace.HTML, self._elem_name_tbody)
        self._html_elem_tfoot            = (Namespace.HTML, self._elem_name_tfoot)
        self._html_elem_tr               = (Namespace.HTML, self._elem_name_tr)
        self._html_elem_td               = (Namespace.HTML, self._elem_name_td)
        self._html_elem_th               = (Namespace.HTML, self._elem_name_th)
        self._html_elem_form             = (Namespace.HTML, self._elem_name_form)
        self._html_elem_input            = (Namespace.HTML, self._elem_name_input)
        self._html_elem_button           = (Namespace.HTML, self._elem_name_button)
        self._html_elem_select           = (Namespace.HTML, self._elem_name_select)
        self._html_elem_optgroup         = (Namespace.HTML, self._elem_name_optgroup)
        self._html_elem_option           = (Namespace.HTML, self._elem_name_option)
        self._html_elem_textarea         = (Namespace.HTML, self._elem_name_textarea)
        self._html_elem_fieldset         = (Namespace.HTML, self._elem_name_fieldset)
        self._html_elem_details          = (Namespace.HTML, self._elem_name_details)
        self._html_elem_summary          = (Namespace.HTML, self._elem_name_summary)
        self._html_elem_script           = (Namespace.HTML, self._elem_name_script)
        self._html_elem_noscript         = (Namespace.HTML, self._elem_name_noscript)
        self._html_elem_template         = (Namespace.HTML, self._elem_name_template)
        self._html_elem_applet           = (Namespace.HTML, self._elem_name_applet)
        self._html_elem_bgsound          = (Namespace.HTML, self._elem_name_bgsound)
        self._html_elem_dir              = (Namespace.HTML, self._elem_name_dir)
        self._html_elem_frame            = (Namespace.HTML, self._elem_name_frame)
        self._html_elem_frameset         = (Namespace.HTML, self._elem_name_frameset)
        self._html_elem_noframes         = (Namespace.HTML, self._elem_name_noframes)
        self._html_elem_keygen           = (Namespace.HTML, self._elem_name_keygen)
        self._html_elem_listing          = (Namespace.HTML, self._elem_name_listing)
        self._html_elem_noembed          = (Namespace.HTML, self._elem_name_noembed)
        self._html_elem_param            = (Namespace.HTML, self._elem_name_param)
        self._html_elem_plaintext        = (Namespace.HTML, self._elem_name_plaintext)
        self._html_elem_rb               = (Namespace.HTML, self._elem_name_rb)
        self._html_elem_rtc              = (Namespace.HTML, self._elem_name_rtc)
        self._html_elem_xmp              = (Namespace.HTML, self._elem_name_xmp)
        self._html_elem_basefont         = (Namespace.HTML, self._elem_name_basefont)
        self._html_elem_center           = (Namespace.HTML, self._elem_name_center)
        self._html_elem_marquee          = (Namespace.HTML, self._elem_name_marquee)
        self._html_elem_nobr             = (Namespace.HTML, self._elem_name_nobr)

        self._mathml_elem_mi             = (Namespace.MATHML, self._elem_name_mi)
        self._mathml_elem_mn             = (Namespace.MATHML, self._elem_name_mn)
        self._mathml_elem_mo             = (Namespace.MATHML, self._elem_name_mo)
        self._mathml_elem_mtext          = (Namespace.MATHML, self._elem_name_mtext)
        self._mathml_elem_ms             = (Namespace.MATHML, self._elem_name_ms)
        self._mathml_elem_annotation_xml = (Namespace.MATHML, self._elem_name_annotation_xml)

        self._svg_elem_desc              = (Namespace.SVG, self._elem_name_desc)
        self._svg_elem_title             = (Namespace.SVG, self._elem_name_title)
        self._svg_elem_foreign_object    = (Namespace.SVG, self._elem_name_foreign_object)
        self._svg_elem_script            = (Namespace.SVG, self._elem_name_script)

        # FIXME: Consider having this be a flag on the element definition instead (another option is a boolean list indexed by element index)                                                                                  
        self._special_elements = {self._html_elem_html, self._html_elem_head, self._html_elem_title,
                                  self._html_elem_base, self._html_elem_link, self._html_elem_meta,
                                  self._html_elem_style, self._html_elem_body, self._html_elem_article,
                                  self._html_elem_section, self._html_elem_nav, self._html_elem_aside,
                                  self._html_elem_h1, self._html_elem_h2, self._html_elem_h3, self._html_elem_h4,
                                  self._html_elem_h5, self._html_elem_h6, self._html_elem_hgroup,
                                  self._html_elem_header, self._html_elem_footer, self._html_elem_address,
                                  self._html_elem_p, self._html_elem_hr, self._html_elem_pre,
                                  self._html_elem_blockquote, self._html_elem_ol, self._html_elem_ul,
                                  self._html_elem_menu, self._html_elem_li, self._html_elem_dl, self._html_elem_dt,
                                  self._html_elem_dd, self._html_elem_figure, self._html_elem_figcaption,
                                  self._html_elem_main, self._html_elem_search, self._html_elem_div,
                                  self._html_elem_br, self._html_elem_wbr, self._html_elem_source, self._html_elem_img,
                                  self._html_elem_iframe, self._html_elem_embed, self._html_elem_object,
                                  self._html_elem_track, self._html_elem_area, self._html_elem_table,
                                  self._html_elem_caption, self._html_elem_colgroup, self._html_elem_col,
                                  self._html_elem_thead, self._html_elem_tbody, self._html_elem_tfoot,
                                  self._html_elem_tr, self._html_elem_td, self._html_elem_th, self._html_elem_form,
                                  self._html_elem_input, self._html_elem_button, self._html_elem_select,
                                  self._html_elem_textarea, self._html_elem_fieldset, self._html_elem_details,
                                  self._html_elem_summary, self._html_elem_script, self._html_elem_noscript,
                                  self._html_elem_template, self._html_elem_applet, self._html_elem_bgsound,
                                  self._html_elem_dir, self._html_elem_frame, self._html_elem_frameset,
                                  self._html_elem_noframes, self._html_elem_keygen, self._html_elem_listing,
                                  self._html_elem_noembed, self._html_elem_param, self._html_elem_plaintext,
                                  self._html_elem_xmp, self._html_elem_basefont, self._html_elem_center,
                                  self._html_elem_marquee, self._mathml_elem_mi, self._mathml_elem_mn,
                                  self._mathml_elem_mo, self._mathml_elem_mtext, self._mathml_elem_ms,
                                  self._mathml_elem_annotation_xml, self._svg_elem_desc, self._svg_elem_title,
                                  self._svg_elem_foreign_object}

        self._default_scope = {self._html_elem_html, self._html_elem_object, self._html_elem_table,
                               self._html_elem_caption, self._html_elem_td, self._html_elem_th,
                               self._html_elem_template, self._html_elem_applet, self._html_elem_marquee,
                               self._mathml_elem_mi, self._mathml_elem_mn, self._mathml_elem_mo,
                               self._mathml_elem_mtext, self._mathml_elem_ms, self._mathml_elem_annotation_xml,
                               self._svg_elem_desc, self._svg_elem_title, self._svg_elem_foreign_object}

        self._list_item_scope =  self._default_scope | {self._html_elem_ol, self._html_elem_ul}

        self._button_scope = self._default_scope | {self._html_elem_button}

        self._implicitly_closeable = {self._html_elem_html, self._html_elem_body, self._html_elem_p,
                                      self._html_elem_li, self._html_elem_dt, self._html_elem_dd, self._html_elem_rt,
                                      self._html_elem_rp, self._html_elem_thead, self._html_elem_tbody,
                                      self._html_elem_tfoot, self._html_elem_tr, self._html_elem_td,
                                      self._html_elem_th, self._html_elem_optgroup, self._html_elem_option,
                                      self._html_elem_rb, self._html_elem_rtc}

        self._svg_element_map = {
            "altglyph":            "altGlyph",
            "altglyphdef":         "altGlyphDef",
            "altglyphitem":        "altGlyphItem",
            "animatecolor":        "animateColor",
            "animatemotion":       "animateMotion",
            "animatetransform":    "animateTransform",
            "clippath":            "clipPath",
            "feblend":             "feBlend",
            "fecolormatrix":       "feColorMatrix",
            "fecomponenttransfer": "feComponentTransfer",
            "fecomposite":         "feComposite",
            "feconvolvematrix":    "feConvolveMatrix",
            "fediffuselighting":   "feDiffuseLighting",
            "fedisplacementmap":   "feDisplacementMap",
            "fedistantlight":      "feDistantLight",
            "fedropshadow":        "feDropShadow",
            "feflood":             "feFlood",
            "fefunca":             "feFuncA",
            "fefuncb":             "feFuncB",
            "fefuncg":             "feFuncG",
            "fefuncr":             "feFuncR",
            "fegaussianblur":      "feGaussianBlur",
            "feimage":             "feImage",
            "femerge":             "feMerge",
            "femergenode":         "feMergeNode",
            "femorphology":        "feMorphology",
            "feoffset":            "feOffset",
            "fepointlight":        "fePointLight",
            "fespecularlighting":  "feSpecularLighting",
            "fespotlight":         "feSpotLight",
            "fetile":              "feTile",
            "feturbulence":        "feTurbulence",
            "foreignobject":       "foreignObject",
            "glyphref":            "glyphRef",
            "lineargradient":      "linearGradient",
            "radialgradient":      "radialGradient",
            "textpath":            "textPath",
        }

        self._svg_attribute_map = {
            "attributename":       "attributeName",
            "attributetype":       "attributeType",
            "basefrequency":       "baseFrequency",
            "baseprofile":         "baseProfile",
            "calcmode":            "calcMode",
            "clippathunits":       "clipPathUnits",
            "diffuseconstant":     "diffuseConstant",
            "edgemode":            "edgeMode",
            "filterunits":         "filterUnits",
            "glyphref":            "glyphRef",
            "gradienttransform":   "gradientTransform",
            "gradientunits":       "gradientUnits",
            "kernelmatrix":        "kernelMatrix",
            "kernelunitlength":    "kernelUnitLength",
            "keypoints":           "keyPoints",
            "keysplines":          "keySplines",
            "keytimes":            "keyTimes",
            "lengthadjust":        "lengthAdjust",
            "limitingconeangle":   "limitingConeAngle",
            "markerheight":        "markerHeight",
            "markerunits":         "markerUnits",
            "markerwidth":         "markerWidth",
            "maskcontentunits":    "maskContentUnits",
            "maskunits":           "maskUnits",
            "numoctaves":          "numOctaves",
            "pathlength":          "pathLength",
            "patterncontentunits": "patternContentUnits",
            "patterntransform":    "patternTransform",
            "patternunits":        "patternUnits",
            "pointsatx":           "pointsAtX",
            "pointsaty":           "pointsAtY",
            "pointsatz":           "pointsAtZ",
            "preservealpha":       "preserveAlpha",
            "preserveaspectratio": "preserveAspectRatio",
            "primitiveunits":      "primitiveUnits",
            "refx":                "refX",
            "refy":                "refY",
            "repeatcount":         "repeatCount",
            "repeatdur":           "repeatDur",
            "requiredextensions":  "requiredExtensions",
            "requiredfeatures":    "requiredFeatures",
            "specularconstant":    "specularConstant",
            "specularexponent":    "specularExponent",
            "spreadmethod":        "spreadMethod",
            "startoffset":         "startOffset",
            "stddeviation":        "stdDeviation",
            "stitchtiles":         "stitchTiles",
            "surfacescale":        "surfaceScale",
            "systemlanguage":      "systemLanguage",
            "tablevalues":         "tableValues",
            "targetx":             "targetX",
            "targety":             "targetY",
            "textlength":          "textLength",
            "viewbox":             "viewBox",
            "viewtarget":          "viewTarget",
            "xchannelselector":    "xChannelSelector",
            "ychannelselector":    "yChannelSelector",
            "zoomandpan":          "zoomAndPan",
        }

        self._foreign_attribute_map = {
            "xlink:actuate": (Namespace.XLINK, "xlink", "actuate"),
            "xlink:arcrole": (Namespace.XLINK, "xlink", "arcrole"),
            "xlink:href":    (Namespace.XLINK, "xlink", "href"),
            "xlink:role":    (Namespace.XLINK, "xlink", "role"),
            "xlink:show":    (Namespace.XLINK, "xlink", "show"),
            "xlink:title":   (Namespace.XLINK, "xlink", "title"),
            "xlink:type":    (Namespace.XLINK, "xlink", "type"),
            "xml:lang":      (Namespace.XML,   "xml",   "lang"),
            "xml:space":     (Namespace.XML,   "xml",   "space"),
            "xmlns":         (Namespace.XMLNS, None,    "xmlns"),
            "xmlns:xlink":   (Namespace.XMLNS, "xmlns", "xlink"),
        }

    def _register_element_name(self, local_name):
        return self._element_names_basis.register(local_name)


class Source:
    # Returns the next line including the final newline character (`\n`). Returns `None`
    # when no more lines remain. The last returned line may or may not have a final newline
    # character. No returned line can be the empty string. Zero lines can be returned
    # altogether.
    def read_line(self):
        raise NotImplementedError()

    def reached_line_number(self, line_number):
        pass


class StringSource(Source):
    def __init__(self, string):
        Source.__init__(self)
        lines = []
        i = 0
        while True:
            j = string.find("\n", i)
            if j < 0:
                break
            j += 1
            lines.append(string[i:j])
            i = j
        if i < len(string):
            lines.append(string[i:])
        self._lines = iter(lines)

    def read_line(self):
        return next(self._lines, None)


class RetainingSource(Source):
    def __init__(self, subsource):
        Source.__init__(self)
        self._subsource = subsource
        self._base_line_number = 1
        self._lines = collections.deque()

    def read_line(self):
        line = self._subsource.read_line()
        if line is not None:
            self._lines.append(line)
        return line

    def reached_line_number(self, line_number):
        self._subsource.reached_line_number(line_number)
        while self._base_line_number < line_number:
            self._lines.popleft()
            self._base_line_number += 1

    def get_line(self, line_number):
        if line_number < self._base_line_number:
            return None
        i = line_number - self._base_line_number
        n = len(self._lines)
        assert i <= n
        return self._lines[i] if i < n else ""


class Callbacks:
    def create_doctype(self, name, public_id, system_id):
        return None

    # `namespace` is one of the values of the `Namespace` enumeration
    # `prefix` is None when there is no namespace prefix
    # `attributes` is a list of quadruples (namespace, prefix, local_name, value)
    # In `attributes`, `namespace` and `prefix` are None when there is no namespace or namespace prefix respectively
    def create_element(self, namespace, prefix, local_name, attributes):
        return None

    def create_text(self, data):
        return None

    def create_comment(self, data):
        return None

    # When this function is called by the parser, `node` is always a node that was
    # previously returned by `Callbacks.create_doctype()`, `Callbacks.create_element()`,
    # `Callbacks.create_text()`, or `Callbacks.create_comment()`.
    #
    # When this function is called by the parser, `parent` is always either the document
    # passed to `Parser.parse()` or one of the elements that were previously returned by
    # `Callbacks.create_element()`.
    #
    # When this function is called by the parser, passed nodes (`node`) other than elements
    # (other than something returned by `Callbacks.create_element()`) will never already
    # have a parent.
    #
    # The parser promises to never request insertion of a text node (something returned by
    # `Callbacks.create_text()`) into a document node (that which was passed to
    # `Parser.parse()`).
    #
    # The parser promises to never request insertion of a document type node (something
    # returned by `Callbacks.create_doctype()`) into an element node (something returned by
    # `Callbacks.create_element()`).
    #
    # In order for a tree-building application to comply with the HTML standard, the callee
    # must adhere to the following rules:
    #
    #  - If `node` is currently a child of some parent, the callee must first remove `node`
    #    as a child of that parent
    #
    #  - If `node` is a text node, and insertion would place the text node immediately after
    #    a preexisting text node in the parent, the callee must instead append the contents
    #    of the incoming text node to the preexisting one.
    #
    #  - When the parent is the document node, restrictions apply as to which childen can be
    #    inserted and where they can be inserted (no text nodes, at most one document type
    #    node, at most one element node, no document type node after element node). The
    #    parser may request insertions that would violate these restrictions. It is the
    #    responsibility of the application to detect and ignore such requests. The
    #    responsibility falls on the application because the parser generally does not know
    #    the true state of the tree (the document passed to `Parser.parse()` could have been
    #    nonempty, or the tree could have been mutated through scripting).
    #
    # FIXME: Consider the case where `node` is `parent` or an ancenstor of `parent`. Can
    # such a request ever be generated by the parser when allowing for mutation of tree by
    # scripts? If it can be generated, can it be caught and aborted by the parser? Probably
    # not. If not, can the responsibility fall on the application to detect and ignore that
    # case?                                                                                          
    #
    def append_child(self, node, parent):
        pass

    # Both `old_parent` and `new_parent` is element nodes.
    def move_children(self, old_parent, new_parent):
        pass


class TreeBuilder(Callbacks):
    def __init__(self, document):
        self._document = document

    def create_doctype(self, name, public_id, system_id):
        return archon.dom.create_document_type(self._document, name, public_id, system_id)

    def create_element(self, namespace, prefix, local_name, attributes):
        assert namespace is not None
        namespace_uri = get_namespace_uri(namespace)
        attributes_2 = []
        for namespace_2, prefix_2, local_name_2, value in attributes:
            namespace_uri_2 = get_namespace_uri(namespace_2)
            attr = archon.dom.create_attribute(self._document, namespace_uri_2, prefix_2, local_name_2, value)
            attributes_2.append(attr)
        return archon.dom.create_element(self._document, namespace_uri, prefix, local_name, attributes_2)

    def create_text(self, data):
        return self._document.create_text_node(data)

    def create_comment(self, data):
        return self._document.create_comment(data)

    def append_child(self, node, parent):
        # FIXME: If parent is a document node, detect and ignore invalid insertions        
        if isinstance(node, archon.dom.Text) and parent.has_child_nodes():
            assert not node.get_parent_node()
            node_2 = parent.get_last_child()
            if isinstance(node_2, archon.dom.Text):
                data = node_2.get_data()
                node_2.set_data(data + node.get_data())
                return
        parent.append_child(node)

    def move_children(self, old_parent, new_parent):
        children = list(old_parent.get_child_nodes()) # Copy to allow mutation during iteration
        for node in children:
            new_parent.append_child(node)


class ErrorHandler:
    def error(location, message):
        pass


class SimpleErrorHandler(ErrorHandler):
    def __init__(self, source_ident, logger):
        ErrorHandler.__init__(self)
        self._source_ident = source_ident
        self._logger = logger

    def error(self, location, message):
        self._logger.log("%s:%s:%s: ERROR: %s", self._source_ident, location.line_number, location.position, message)


class ContextShowingErrorHandler(ErrorHandler):
    def __init__(self, retaining_source, error_subhandler, logger):
        self._retaining_source = retaining_source
        self._error_subhandler = error_subhandler
        self._logger = logger

    def error(self, location, message):
        self._error_subhandler.error(location, message)
        line = self._retaining_source.get_line(location.line_number)
        if line is None:
            return
        if line and line[-1] == "\n":
            line = line[:-1]
        max_size = 90
        snippet, position = _extract_snippet(line, location.position, max_size)
        # FIXME: What if some characters are UTF-8 multi-byte sequences?    
        # FIXME: The position returned as second component by getpos() of html.parser.HTMLParser, does it count bytes or characters?                 
        self._logger.log("> %s", snippet)
        self._logger.log("  %s^", position * " ")


class Namespace(enum.Enum):
    HTML   = enum.auto()
    MATHML = enum.auto()
    SVG    = enum.auto()
    XLINK  = enum.auto()
    XML    = enum.auto()
    XMLNS  = enum.auto()


def get_namespace_uri(namespace):
    if namespace is None:
        return None
    if namespace == Namespace.HTML:
        return "http://www.w3.org/1999/xhtml"
    if namespace == Namespace.MATHML:
        return "http://www.w3.org/1998/Math/MathML"
    if namespace == Namespace.SVG:
        return "http://www.w3.org/2000/svg"
    if namespace == Namespace.XLINK:
        return "http://www.w3.org/1999/xlink"
    if namespace == Namespace.XML:
        return "http://www.w3.org/XML/1998/namespace"
    if namespace == Namespace.XMLNS:
        return "http://www.w3.org/2000/xmlns/"
    assert False


class _Session:
    def __init__(self, parser, source, document, callbacks, error_handler, logger):
        self._tokenizer = _Tokenizer(self)
        self._parser = parser
        self._source = source
        self._document = _Document(document)
        self._callbacks = callbacks
        self._error_handler = error_handler
        self._logger = logger
        self._element_names = _StringRegistry(parser._element_names_basis)
        self._insertion_mode = _InsertionMode.INITIAL
        self._ignore_linefeed = False
        self._selfclosing_flag_acknowledged = False
        self._frameset_ok = True

        # The stack of open elements, i.e., `self._open_elements` is in general the path
        # from the root to the element to which the parser is currently appending
        # children. Only script execution during parsing can change this, and since
        # there is currently no support for scripting, the stack of open elements is in
        # fact always that path.
        #
        # The list of active formatting elements, i.e.,
        # `self._active_formatting_elements` is used to deal with misnested formatting
        # element tags. It is generally the list of formatting elements that are either
        # currently open or have been open and are now implicitely closed. A formatting
        # element is implicitely closed if it is closed before the end tag has occurred.
        #
        # INVARIANT: Elements, that occur in both `self._open_elements` and
        # `self._active_formatting_elements`, occur in the same order in the two places.
        #
        self._open_elements = []
        self._active_formatting_elements = []

    def run(self):
        while True:
            line = self._source.read_line()
            if line is None:
                break
            self._tokenizer.feed(line)
        self.handle_end_of_input()

    def handle_doctype(self, decl):
        self._log("HANDLE: <!doctype>")
        location = self._get_location()
        # FIXME: Newline normalization!?!?    
        name, public_id, system_id, force_quirks_mode = self._parse_doctype(location, decl)
        self._process_token(_Doctype(location, name, public_id, system_id, force_quirks_mode))

    def handle_data(self, data):
        self._log("HANDLE: DATA")
        location = self._get_location()
        # FIXME: Consider https://html.spec.whatwg.org/#preprocessing-the-input-stream         
        assert data
        if self._ignore_linefeed:
            if data[0] == "\n":
                data = data[1:]
                if not data:
                    return
                location.line_number += 1
                location.position = 0
            self._ignore_linefeed = False
        self._process_token(_Data(location, data))

    def handle_starttag(self, tag, attrs, selfclosing = False):
        self._log("HANDLE: <%s>", tag)
        location = self._get_location()
        elem_name_index = self._element_names.intern(tag)
        attributes = self._build_attributes(location, attrs)
        selfclosing = False
        self._process_token(_StartTag(location, elem_name_index, attributes, selfclosing))

    def handle_endtag(self, tag):
        self._log("HANDLE: </%s>", tag)
        location = self._get_location()
        elem_name_index = self._element_names.intern(tag)
        self._process_token(_EndTag(location, elem_name_index))

    def handle_startendtag(self, tag, attrs):
        self._log("HANDLE: <%s/>", tag)
        location = self._get_location()
        elem_name_index = self._element_names.intern(tag)
        attributes = self._build_attributes(location, attrs)
        selfclosing = True
        self._selfclosing_flag_acknowledged = False
        self._process_token(_StartTag(location, elem_name_index, attributes, selfclosing))
        if not self._selfclosing_flag_acknowledged:
            self._parse_error(location, "Error (non-void-html-element-start-tag-with-trailing-solidus)")    

    def handle_comment(self, data):
        self._log("HANDLE: <!-- -->")
        location = self._get_location()
        self._process_token(_Comment(location, data))

    def handle_end_of_input(self):
        self._log("HANDLE: End of input")
        location = self._get_location()
        self._process_token(_EndOfInput(location))
        self._stop_parsing()

    def _get_location(self):
        line_number, position = self._tokenizer.getpos()
        return Location(line_number, position)

    def _parse_doctype(self, location, text):
        text_2 = text.replace("\0", "\u0394")
        if text_2 != text:
            # FIXME: Point to first null character        
            self._parse_error(location, "Error (unexpected-null-character)")
        i = 0
        n = len(text_2)
        def begins_with(prefix):
            j = i + len(prefix)
            return text_2[i:j].lower() == prefix.lower()
        def try_consume(prefix):
            nonlocal i
            if begins_with(prefix):
                i += len(prefix)
                return True
            return False
        def skip_whitespace():
            nonlocal i
            i_0 = i
            # FIXME: Carriage return (`\r`) is here because of missing newline normalization    
            while i < n and text_2[i] in "\t\n\f\r ":
                i += 1
            return i - i_0
        def skip_nonwhitespace():
            nonlocal i
            while i < n and text_2[i] not in "\t\n\f\r ":
                i += 1
        def ascii_lower(text):
            return "".join([ch.lower() if ch.isascii() else ch for ch in text])
        name = None
        public_id = None
        system_id = None
        force_quirks_mode = False
        def consume_public_id():
            nonlocal i, public_id, force_quirks_mode
            assert i < n
            mark = text_2[i]
            if mark not in "'\"":
                self._parse_error(location, "Error (missing-quote-before-doctype-public-identifier)")
                force_quirks_mode = True
                return False
            i += 1
            j = text_2.find(mark, i)
            unclosed = False
            if j == -1:
                unclosed = True
                j == n
            assert j >= i
            public_id = text_2[i:j]
            if unclosed:
                self._parse_error(location, "Error (abrupt-doctype-public-identifier)")
                force_quirks_mode = True
                return False
            i = j + 1
            return True
        def consume_system_id():
            nonlocal i, system_id, force_quirks_mode
            assert i < n
            mark = text_2[i]
            if mark not in "'\"":
                self._parse_error(location, "Error (missing-quote-before-doctype-system-identifier)")
                force_quirks_mode = True
                return
            i += 1
            j = text_2.find(mark, i)
            unclosed = False
            if j == -1:
                unclosed = True
                j == n
            assert j >= i
            system_id = text_2[i:j]
            if unclosed:
                self._parse_error(location, "Error (abrupt-doctype-system-identifier)")
                force_quirks_mode = True
                return
            i = j + 1
            skip_whitespace()
            if i < n:
                self._parse_error(location, "Error (unexpected-character-after-doctype-system-identifier)")
        def parse():
            nonlocal i, name, force_quirks_mode
            assert try_consume("DOCTYPE")
            m = skip_whitespace()
            if i == n:
                self._parse_error(location, "Error (missing-doctype-name)")
                force_quirks_mode = True
                return
            if m == 0:
                self._parse_error(location, "Error (missing-whitespace-before-doctype-name)")
            i_0 = i
            i += 1
            skip_nonwhitespace()
            name = ascii_lower(text_2[i_0:i])
            skip_whitespace()
            if try_consume("PUBLIC"):
                if i < n and text_2[i] in "'\"":
                    self._parse_error(location, "Error (missing-whitespace-after-doctype-public-keyword)")
                skip_whitespace()
                if i == n:
                    self._parse_error(location, "Error (missing-doctype-public-identifier)")
                    force_quirks_mode = True
                    return
                if consume_public_id():
                    if i < n and text_2[i] in "'\"":
                        self._parse_error(location, "Error "
                                          "(missing-whitespace-between-doctype-public-and-system-identifiers)")
                    skip_whitespace()
                    if i < n:
                        consume_system_id()
                return
            if try_consume("SYSTEM"):
                if i < n and text_2[i] in "'\"":
                    self._parse_error(location, "Error (missing-whitespace-after-doctype-system-keyword)")
                skip_whitespace()
                if i == n:
                    self._parse_error(location, "Error (missing-doctype-system-identifier)")
                    force_quirks_mode = True
                    return
                consume_system_id()
                return
            if i < n:
                self._parse_error(location, "Error (invalid-character-sequence-after-doctype-name)")
                force_quirks_mode = True
        parse()
        return name, public_id, system_id, force_quirks_mode

    def _build_attributes(self, location, attrs):
        attributes = []
        seen_attributes = set()
        duplicates_seen = False
        for name, value in attrs:
            if name in seen_attributes:
                duplicates_seen = True
            else:
                namespace = None
                prefix = None
                value_2 = value if value is not None else ""
                attr = _Attribute(namespace, prefix, name, value_2)
                attributes.append(attr)
                seen_attributes.add(name)
        if duplicates_seen:
            self._parse_error(location, "Invalid start tag: Duplicate attributes (first occurrences win)")
        return attributes

    def _process_token(self, token):
        self._source.reached_line_number(token.location.line_number)

        parser = self._parser
        is_html = False
        elem = self._get_adjusted_current_element()
        if not elem:
            is_html = True
        elif elem.namespace == Namespace.HTML:
            is_html = True
        elif self._is_mathml_text_integration_point(elem):
            if isinstance(token, _StartTag):
                if token.elem_name_index not in [parser._elem_name_mglyph, parser._elem_name_malignmark]:
                    is_html = True
            elif isinstance(token, _Data):
                is_html = True
        elif elem.get_type() == parser._mathml_elem_annotation_xml:
            if isinstance(token, _StartTag) and token.elem_name_index == parser._elem_name_svg:
                is_html = True
        elif self._is_html_integration_point(elem):
            if isinstance(token, _StartTag) or isinstance(token, _Data):
                is_html = True
        elif isinstance(token, _EndOfInput):
            is_html = True

        if is_html:
            self._process_html_token(token)
        else:
            self._process_foreign_token(token)

    def _process_html_token(self, token):
        while True:
            processed = self._do_process_html_token(token)
            if processed:
                break
            self._log("REPROCESS")

    # This function is allowed to modify the token for reprocessing purposes
    def _do_process_html_token(self, token):
        self._log("MODE: %s", self._insertion_mode.name)
        if self._insertion_mode == _InsertionMode.INITIAL:
            return self._process_token_initial(token)
        if self._insertion_mode == _InsertionMode.BEFORE_HTML:
            return self._process_token_before_html(token)
        if self._insertion_mode == _InsertionMode.BEFORE_HEAD:
            return self._process_token_before_head(token)
        if self._insertion_mode == _InsertionMode.IN_HEAD:
            return self._process_token_in_head(token)
        if self._insertion_mode == _InsertionMode.IN_HEAD_NOSCRIPT:
            return self._process_token_in_head_noscript(token)
        if self._insertion_mode == _InsertionMode.AFTER_HEAD:
            return self._process_token_after_head(token)
        if self._insertion_mode == _InsertionMode.IN_BODY:
            return self._process_token_in_body(token)
        if self._insertion_mode == _InsertionMode.TEXT:
            return self._process_token_text(token)
        if self._insertion_mode == _InsertionMode.IN_TEMPLATE:
            return self._process_token_in_template(token)
        if self._insertion_mode == _InsertionMode.AFTER_BODY:
            return self._process_token_after_body(token)
        if self._insertion_mode == _InsertionMode.IN_FRAMESET:
            return self._process_token_in_frameset(token)
        if self._insertion_mode == _InsertionMode.AFTER_FRAMESET:
            return self._process_token_after_frameset(token)
        if self._insertion_mode == _InsertionMode.AFTER_AFTER_BODY:
            return self._process_token_after_after_body(token)
        if self._insertion_mode == _InsertionMode.AFTER_AFTER_FRAMESET:
            return self._process_token_after_after_frameset(token)
        assert False

    def _process_token_initial(self, token):
        location = token.location
        if isinstance(token, _Doctype):
            # FIXME: If the DOCTYPE token's name is not "html", or the token's public identifier is not missing, or the token's system identifier is neither missing nor "about:legacy-compat", then there is a parse error.                                                                     
            self._insert_doctype(token.name, token.public_id, token.system_id, parent = self._document)
            # FIXME: Deal with quirks mode and limited-quirks mode            
            self._insertion_mode = _InsertionMode.BEFORE_HTML
            return True
        elif isinstance(token, _Data):
            token.skip_leading_whitespace()
            if not token.data:
                return True
        elif isinstance(token, _Comment):
            self._insert_comment(token.data, parent = self._document)
            return True
        # FIXME: If the document is not an iframe srcdoc document and the parser can change the mode flag ("parser cannot change the mode flag" is false), set the Document to quirks mode.    
        # FIXME: If the document is an iframe srcdoc document, then this is not parse error.    
        self._parse_error(location, "Invalid token before <!doctype>")
        self._insertion_mode = _InsertionMode.BEFORE_HTML
        return False # Reprocess

    def _process_token_before_html(self, token):
        location = token.location
        parser = self._parser
        if isinstance(token, _Data):
            token.skip_leading_whitespace()
            if not token.data:
                return True
        elif isinstance(token, _StartTag):
            name_index = token.elem_name_index
            if name_index == parser._elem_name_html:
                assert not self._open_elements
                # FIXME: Assert no "foster parenting"    
                self._open_html_element_for_token(token)
                self._insertion_mode = _InsertionMode.BEFORE_HEAD
                return True
        elif isinstance(token, _EndTag):
            name_index = token.elem_name_index
            if name_index in [parser._elem_name_html, parser._elem_name_head, parser._elem_name_body,
                              parser._elem_name_br]:
                pass
            else:
                self._parse_error(location, "Invalid end tag before <html> (ignored)")
                return True
        elif isinstance(token, _Comment):
            self._insert_comment(token.data, parent = self._document)
            return True
        elif isinstance(token, _Doctype):
            self._bad_doctype(location)
            return True
        assert not self._open_elements
        # FIXME: Assert no "foster parenting"    
        self._open_synthetic_element(parser._html_elem_html)
        self._insertion_mode = _InsertionMode.BEFORE_HEAD
        return False # Reprocess

    def _process_token_before_head(self, token):
        location = token.location
        parser = self._parser
        if isinstance(token, _Data):
            token.skip_leading_whitespace()
            if not token.data:
                return True
        elif isinstance(token, _StartTag):
            name_index = token.elem_name_index
            if name_index == parser._elem_name_head:
                self._open_html_element_for_token(token)
                # FIXME: Set the head element pointer to the newly created head element.              
                self._insertion_mode = _InsertionMode.IN_HEAD
                return True
            elif name_index == parser._elem_name_html:
                return self._process_token_in_body(token)
        elif isinstance(token, _EndTag):
            name_index = token.elem_name_index
            if name_index in [parser._elem_name_html, parser._elem_name_head, parser._elem_name_body,
                              parser._elem_name_br]:
                pass
            else:
                self._parse_error(location, "Invalid end tag before <head> (ignored)")
                return True
        elif isinstance(token, _Comment):
            self._insert_comment(token.data)
            return True
        elif isinstance(token, _Doctype):
            self._bad_doctype(location)
            return True
        self._open_synthetic_element(parser._html_elem_head)
        # FIXME: Set the head element pointer to the newly created head element.              
        self._insertion_mode = _InsertionMode.IN_HEAD
        return False # Reprocess

    def _process_token_in_head(self, token):
        location = token.location
        parser = self._parser
        processed = False
        if isinstance(token, _Data):
            whitespace = token.skip_leading_whitespace()
            if whitespace:
                self._insert_text(whitespace)
            if not token.data:
                return True
        elif isinstance(token, _StartTag):
            name_index = token.elem_name_index
            if name_index == parser._elem_name_title:
                self._open_html_element_for_token(token)
                self._orig_insertion_mode = self._insertion_mode
                self._insertion_mode = _InsertionMode.TEXT
                # FIXME: Need tokenizer mode "RCDATA"    
                return True
            elif name_index in [parser._elem_name_base, parser._elem_name_link, parser._elem_name_bgsound,
                                parser._elem_name_basefont]:
                self._open_html_element_for_token(token)
                self._close_element()
                if token.selfclosing:
                    self._selfclosing_flag_acknowledged = True
                return True
            elif name_index == parser._elem_name_meta:
                self._open_html_element_for_token(token)
                self._close_element()
                if token.selfclosing:
                    self._selfclosing_flag_acknowledged = True
                # FIXME: Deal with switching of character encoding if the active speculative HTML parser is null    
                return True
            elif name_index in [parser._elem_name_style, parser._elem_name_noframes]:
                self._open_html_element_for_token(token)
                self._orig_insertion_mode = self._insertion_mode
                self._insertion_mode = _InsertionMode.TEXT
                # FIXME: Need tokenizer mode "generic_raw_text"    
                return True
            elif name_index == parser._elem_name_script:
                self._open_html_element_for_token(token)
                self._orig_insertion_mode = self._insertion_mode
                self._insertion_mode = _InsertionMode.TEXT
                # FIXME: Need tokenizer mode "script_data"    
                return True
            elif name_index == parser._elem_name_noscript:
                self._open_html_element_for_token(token)
                self._insertion_mode = _InsertionMode.IN_HEAD_NOSCRIPT
                return True
            elif name_index == parser._elem_name_template:
                self._active_formatting_elements.append(None) # Marker
                self._frameset_ok = False
                # # FIXME: self._template_insertion_modes.append(_InsertionMode.IN_TEMPLATE)    
                # self._open_html_element_for_token(token)
                # self._insertion_mode = _InsertionMode.IN_TEMPLATE
                # return True
                raise RuntimeError("Not yet implemented")              
            elif name_index == parser._elem_name_html:
                return self._process_token_in_body(token)
            elif name_index == parser._elem_name_head:
                self._parse_error(location, "Invalid start tag in <head> (ignored)")
                return True
        elif isinstance(token, _EndTag):
            name_index = token.elem_name_index
            if name_index == parser._elem_name_head:
                elem = self._close_element()
                assert elem.get_type() == parser._html_elem_head
                self._insertion_mode = _InsertionMode.AFTER_HEAD
                return True
            elif name_index in [parser._elem_name_html, parser._elem_name_body, parser._elem_name_br]:
                pass
            elif name_index == parser._elem_name_template:
                raise RuntimeError("Not yet implemented")              
            else:
                self._parse_error(location, "Invalid end tag in <head> (ignored)")
                return True
        elif isinstance(token, _Comment):
            self._insert_comment(token.data)
            return True
        elif isinstance(token, _Doctype):
            self._bad_doctype(location)
            return True
        elem = self._close_element()
        assert elem.get_type() == parser._html_elem_head
        self._insertion_mode = _InsertionMode.AFTER_HEAD
        return False # Reprocess

    def _process_token_in_head_noscript(self, token):
        location = token.location
        parser = self._parser
        if isinstance(token, _Data):
            whitespace = token.skip_leading_whitespace()
            if whitespace:
                self._insert_text(whitespace)
            if not token.data:
                return True
        elif isinstance(token, _StartTag):
            name_index = token.elem_name_index
            if name_index in [parser._elem_name_link, parser._elem_name_meta, parser._elem_name_style,
                              parser._elem_name_bgsound, parser._elem_name_noframes, parser._elem_name_basefont]:
                return self._process_token_in_head(token)
            elif name_index == parser._elem_name_html:
                return self._process_token_in_body(token)
            elif name_index in [parser._elem_name_head, parser._elem_name_noscript]:
                self._parse_error(location, "Invalid start tag inside <noscript> element inside <head> element (start "
                                  "tag ignored)")
                return True
        elif isinstance(token, _EndTag):
            name_index = token.elem_name_index
            if name_index == parser._elem_name_noscript:
                elem = self._close_element()
                assert elem.get_type() == parser._html_elem_noscript
                self._insertion_mode = _InsertionMode.IN_HEAD
                return True
            elif name_index == parser._elem_name_br:
                pass
            else:
                self._parse_error(location, "Invalid end tag inside <noscript> element inside <head> element (end tag "
                                  "ignored)")
                return True
        elif isinstance(token, _Comment):
            self._insert_comment(token.data)
            return True
        elif isinstance(token, _Doctype):
            self._bad_doctype(location)
            return True
        self._parse_error(location, "Invalid token inside <noscript> element inside <head> element (<noscript> "
                          "element force-closed)")
        elem = self._close_element()
        assert elem.get_type() == parser._html_elem_noscript
        self._insertion_mode = _InsertionMode.IN_HEAD
        return False # Reprocess

    def _process_token_after_head(self, token):
        location = token.location
        parser = self._parser
        if isinstance(token, _Data):
            whitespace = token.skip_leading_whitespace()
            if whitespace:
                self._insert_text(whitespace)
            if not token.data:
                return True
        elif isinstance(token, _StartTag):
            name_index = token.elem_name_index
            if name_index == parser._elem_name_body:
                self._open_html_element_for_token(token)
                self._frameset_ok = False
                self._insertion_mode = _InsertionMode.IN_BODY
                return True
            elif name_index == parser._elem_name_frameset:
                self._open_html_element_for_token(token)
                self._insertion_mode = _InsertionMode.IN_FRAMESET
                return True
            elif name_index in [parser._elem_name_title, parser._elem_name_base, parser._elem_name_link,
                                parser._elem_name_meta, parser._elem_name_style, parser._elem_name_script,
                                parser._elem_name_template, parser._elem_name_bgsound, parser._elem_name_noframes,
                                parser._elem_name_basefont]:
                # FIXME: Push the node pointed to by the head element pointer onto the stack of open elements.    
                # FIXME: return self._process_token_in_body(token)    
                # FIXME: Remove the node pointed to by the head element pointer from the stack of open elements. (It might not be the current node at this point.)    
                raise RuntimeError("Not yet implemented")              
            elif name_index == parser._elem_name_html:
                return self._process_token_in_body(token)
            elif name_index == parser._elem_name_head:
                self._parse_error(location, "Start tag for <head> after <head> (ignored)")
                return True
        elif isinstance(token, _EndTag):
            name_index = token.elem_name_index
            if name_index in [parser._elem_name_html, parser._elem_name_body, parser._elem_name_br]:
                pass
            elif name_index == parser._elem_name_template:
                return self._process_token_in_head(token)
            else:
                self._parse_error(location, "Invalid end tag between <head> and <body> (ignored)")
                return True
        elif isinstance(token, _Comment):
            self._insert_comment(token.data)
            return True
        elif isinstance(token, _Doctype):
            self._bad_doctype(location)
            return True
        self._open_synthetic_element(parser._html_elem_body)
        self._insertion_mode = _InsertionMode.IN_BODY
        return False # Reprocess

    def _process_token_in_body(self, token):
        location = token.location
        parser = self._parser
        if isinstance(token, _Data):
            data = token.data.replace("\0", "")
            if len(data) < len(token.data):
                # FIXME: Point to first null character    
                self._parse_error(location, "Invalid character data: Contains null characters (removed)")
            if not data:
                return True
            if _has_nonwhitespace(data):
                self._frameset_ok = False
            self._reconstruct_active_formatting_elements()
            self._insert_text(data)
            return True
        if isinstance(token, _StartTag):
            name_index = token.elem_name_index
            if name_index in [parser._elem_name_article, parser._elem_name_section, parser._elem_name_nav,
                              parser._elem_name_aside, parser._elem_name_hgroup, parser._elem_name_header,
                              parser._elem_name_footer, parser._elem_name_address, parser._elem_name_p,
                              parser._elem_name_blockquote, parser._elem_name_ol, parser._elem_name_ul,
                              parser._elem_name_menu, parser._elem_name_dl, parser._elem_name_figure,
                              parser._elem_name_figcaption, parser._elem_name_main, parser._elem_name_search,
                              parser._elem_name_div, parser._elem_name_fieldset, parser._elem_name_details,
                              parser._elem_name_summary, parser._elem_name_dialog, parser._elem_name_dir,
                              parser._elem_name_center]:
                if self._find_element_in_scope(parser._html_elem_p, scope = parser._button_scope):
                    self._close_a_p_element(location)
                self._open_html_element_for_token(token)
                return True
            elif name_index in [parser._elem_name_h1, parser._elem_name_h2, parser._elem_name_h3,
                                parser._elem_name_h4, parser._elem_name_h5, parser._elem_name_h6]:
                if self._find_element_in_scope(parser._html_elem_p, scope = parser._button_scope):
                    self._close_a_p_element(location)
                elem = self._open_elements[-1]
                if elem.get_type() in [parser._html_elem_h1, parser._html_elem_h2, parser._html_elem_h3,
                                       parser._html_elem_h4, parser._html_elem_h5, parser._html_elem_h6]:
                    self._parse_error(location, "Error 1")    
                self._open_html_element_for_token(token)
                return True
            elif name_index in [parser._elem_name_pre, parser._elem_name_listing]:
                if self._find_element_in_scope(parser._html_elem_p, scope = parser._button_scope):
                    self._close_a_p_element(location)
                self._open_html_element_for_token(token)
                self._ignore_linefeed = True
                self._frameset_ok = False
                return True
            elif name_index == parser._elem_name_form:
                # FIXME: If the form element pointer is not null, and there is no template element on the stack of open elements, then this is a parse error; ignore the token.    
                if self._find_element_in_scope(parser._html_elem_p, scope = parser._button_scope):
                    self._close_a_p_element(location)
                self._open_html_element_for_token(token)
                # FIXME: If there is no template element on the stack of open elements, set the form element pointer to point to the element created.    
                raise RuntimeError("Not yet implemented")              
            elif name_index == parser._elem_name_li:
                self._frameset_ok = False
                for elem in reversed(self._open_elements):
                    if elem.get_type() == parser._html_elem_li:
                        self._generate_implied_end_tags(exception = parser._html_elem_li)
                        if self._open_elements[-1].get_type() != parser._html_elem_li:
                            self._parse_error(location, "<li> start tag inside uncloseable <li> element")
                        while True:
                            elem = self._close_element()
                            if elem.get_type() == parser._html_elem_li:
                                break
                        break
                    if elem.get_type() in parser._special_elements:
                        if elem.get_type() not in [parser._html_elem_address, parser._html_elem_p,
                                                   parser._html_elem_div]:
                            break
                if self._find_element_in_scope(parser._html_elem_p, scope = parser._button_scope):
                    self._close_a_p_element(location)
                self._open_html_element_for_token(token)
                return True
            elif name_index in [parser._elem_name_dt, parser._elem_name_dd]:
                raise RuntimeError("Not yet implemented")              
            elif name_index == parser._elem_name_plaintext:
                raise RuntimeError("Not yet implemented")              
            elif name_index == parser._elem_name_button:
                if self._find_element_in_scope(parser._html_elem_button):
                    self._parse_error(location, "<button> start tag inside <button> element (closing prior)")
                    self._generate_implied_end_tags()
                    while True:
                        elem = self._close_element()
                        if elem.get_type() == parser._html_elem_button:
                            break
                self._reconstruct_active_formatting_elements()
                self._open_html_element_for_token(token)
                self._frameset_ok = False
                return True
            elif name_index in [parser._elem_name_title, parser._elem_name_base, parser._elem_name_link,
                                parser._elem_name_meta, parser._elem_name_style, parser._elem_name_script,
                                parser._elem_name_template, parser._elem_name_bgsound, parser._elem_name_noframes,
                                parser._elem_name_basefont]:
                return self._process_token_in_head(token)
            elif name_index == parser._elem_name_a:
                for elem in reversed(self._active_formatting_elements):
                    is_marker = not elem
                    if is_marker:
                        break
                    if elem.get_type() != parser._html_elem_a:
                        continue
                    self._parse_error(location, "<a> start tag inside <a> element")
                    done = self._adoption_agency(location, name_index)
                    if not done:
                        raise RuntimeError("Not yet implemented")                                                                  
                    raise RuntimeError("Not yet implemented")              
                elem = self._open_html_element_for_token(token)
                self._push_active_formatting_element(elem)
                return True
            elif name_index in [parser._elem_name_em, parser._elem_name_strong, parser._elem_name_small,
                                parser._elem_name_s, parser._elem_name_code, parser._elem_name_i,
                                parser._elem_name_b, parser._elem_name_u, parser._elem_name_strike,
                                parser._elem_name_big, parser._elem_name_font, parser._elem_name_tt]:
                self._reconstruct_active_formatting_elements()
                elem = self._open_html_element_for_token(token)
                self._push_active_formatting_element(elem)
                return True
            elif name_index == parser._elem_name_nobr:
                self._reconstruct_active_formatting_elements()
                if self._find_element_in_scope(parser._html_elem_nobr):
                    self._parse_error(location, "<nobr> start tag inside <nobr> element")
                    done = self._adoption_agency(location, name_index)
                    if not done:
                        raise RuntimeError("Not yet implemented")                                                                  
                    self._reconstruct_active_formatting_elements()
                elem = self._open_html_element_for_token(token)
                self._push_active_formatting_element(elem)
                return True
            elif name_index in [parser._elem_name_object, parser._elem_name_applet, parser._elem_name_marquee]:
                self._reconstruct_active_formatting_elements()
                self._open_html_element_for_token(token)
                self._active_formatting_elements.append(None) # Marker
                self._frameset_ok = False
                return True
            elif name_index == parser._elem_name_table:
                raise RuntimeError("Not yet implemented")              
            elif name_index in [parser._elem_name_br, parser._elem_name_wbr, parser._elem_name_img,
                                parser._elem_name_embed, parser._elem_name_area, parser._elem_name_keygen]:
                self._reconstruct_active_formatting_elements()
                self._open_html_element_for_token(token)
                self._close_element()
                if token.selfclosing:
                    self._selfclosing_flag_acknowledged = True
                self._frameset_ok = False
                return True
            elif name_index == parser._elem_name_input:
                self._reconstruct_active_formatting_elements()
                elem = self._open_html_element_for_token(token)
                self._close_element()
                if token.selfclosing:
                    self._selfclosing_flag_acknowledged = True
                attr = elem.attribute_map.get("type")
                if not attr or attr.value.lower() == "hidden":
                    self._frameset_ok = False
                return True
            elif name_index in [parser._elem_name_source, parser._elem_name_track, parser._elem_name_param]:
                self._open_html_element_for_token(token)
                self._close_element()
                if token.selfclosing:
                    self._selfclosing_flag_acknowledged = True
                return True
            elif name_index == parser._elem_name_hr:
                if self._find_element_in_scope(parser._html_elem_p, scope = parser._button_scope):
                    self._close_a_p_element(location)
                self._open_html_element_for_token(token)
                self._close_element()
                if token.selfclosing:
                    self._selfclosing_flag_acknowledged = True
                self._frameset_ok = False
                return True
            elif name_index == parser._elem_name_image:
                self._parse_error(location, "Misspelling of <img> element")
                token.elem_name_index = parser._elem_name_img
                return False # Reprocess
            elif name_index == parser._elem_name_textarea:
                self._open_html_element_for_token(token)
                self._ignore_linefeed = True
                self._frameset_ok = False
                self._orig_insertion_mode = self._insertion_mode
                self._insertion_mode = _InsertionMode.TEXT
                # FIXME: Need tokenizer mode "RCDATA"    
                return True
            elif name_index == parser._elem_name_xmp:
                if self._find_element_in_scope(parser._html_elem_p, scope = parser._button_scope):
                    self._close_a_p_element(location)
                self._reconstruct_active_formatting_elements()
                self._frameset_ok = False
                self._open_html_element_for_token(token)
                self._orig_insertion_mode = self._insertion_mode
                self._insertion_mode = _InsertionMode.TEXT
                # FIXME: Need tokenizer mode "RAWTEXT"    
                return True
            elif name_index == parser._elem_name_iframe:
                self._frameset_ok = False
                self._open_html_element_for_token(token)
                self._orig_insertion_mode = self._insertion_mode
                self._insertion_mode = _InsertionMode.TEXT
                # FIXME: Need tokenizer mode "RAWTEXT"    
                return True
            elif name_index == parser._elem_name_noembed:
                self._open_html_element_for_token(token)
                self._orig_insertion_mode = self._insertion_mode
                self._insertion_mode = _InsertionMode.TEXT
                # FIXME: Need tokenizer mode "RAWTEXT"    
                return True
            elif name_index == parser._elem_name_select:
                raise RuntimeError("Not yet implemented")              
            elif name_index in [parser._elem_name_optgroup, parser._elem_name_option]:
                raise RuntimeError("Not yet implemented")              
            elif name_index in [parser._elem_name_rb, parser._elem_name_rtc]:
                raise RuntimeError("Not yet implemented")              
            elif name_index in [parser._elem_name_rt, parser._elem_name_rp]:
                raise RuntimeError("Not yet implemented")              
            elif name_index == parser._elem_name_math:
                self._reconstruct_active_formatting_elements()
                self._adjust_mathml_attributes(token)
                self._adjust_foreign_attributes(token)
                self._open_element_for_token(token, Namespace.MATHML)
                if token.selfclosing:
                    self._close_element()
                    self._selfclosing_flag_acknowledged = True
                return True
            elif name_index == parser._elem_name_svg:
                self._reconstruct_active_formatting_elements()
                self._adjust_svg_attributes(token)
                self._adjust_foreign_attributes(token)
                self._open_element_for_token(token, Namespace.SVG)
                if token.selfclosing:
                    self._close_element()
                    self._selfclosing_flag_acknowledged = True
                return True
            elif name_index in [parser._elem_name_head, parser._elem_name_caption, parser._elem_name_colgroup,
                                parser._elem_name_col, parser._elem_name_tbody, parser._elem_name_thead,
                                parser._elem_name_tfoot, parser._elem_name_tr, parser._elem_name_td,
                                parser._elem_name_th, parser._elem_name_frame]:
                self._parse_error(location, "Error 2")    
                return True
            elif name_index == parser._elem_name_frameset:
                raise RuntimeError("Not yet implemented")              
            elif name_index == parser._elem_name_body:
                self._parse_error(location, "Invalid <body> start tag: Body has already been implicitly or explicitly "
                                  "opened (new attributes retained)")
                raise RuntimeError("Not yet implemented")              
            elif name_index == parser._elem_name_html:
                raise RuntimeError("Not yet implemented")              
            self._reconstruct_active_formatting_elements()
            self._open_html_element_for_token(token)
            return True
        if isinstance(token, _EndTag):
            name_index = token.elem_name_index
            if name_index == parser._elem_name_template:
                return self._process_token_in_head(token)
            elif name_index in [parser._elem_name_article, parser._elem_name_section, parser._elem_name_nav,
                                parser._elem_name_aside, parser._elem_name_hgroup, parser._elem_name_header,
                                parser._elem_name_footer, parser._elem_name_address, parser._elem_name_pre,
                                parser._elem_name_blockquote, parser._elem_name_ol, parser._elem_name_ul,
                                parser._elem_name_menu, parser._elem_name_dl, parser._elem_name_figure,
                                parser._elem_name_figcaption, parser._elem_name_main, parser._elem_name_search,
                                parser._elem_name_div, parser._elem_name_button, parser._elem_name_fieldset,
                                parser._elem_name_details, parser._elem_name_summary, parser._elem_name_dialog,
                                parser._elem_name_dir, parser._elem_name_listing, parser._elem_name_center]:
                elem_type = (Namespace.HTML, name_index)
                if not self._find_element_in_scope(elem_type):
                    self._parse_error(location, "Invalid end tag: No element to close (ignored)")
                    return True
                self._generate_implied_end_tags()
                if self._open_elements[-1].get_type() != elem_type:
                    self._parse_error(location, "Invalid end tag: Inside element that is not implicitely closeable")
                while True:
                    elem = self._close_element()
                    if elem.get_type() == elem_type:
                        break
                return True
            elif name_index == parser._elem_name_form:
                raise RuntimeError("Not yet implemented")              
            elif name_index == parser._elem_name_p:
                if not self._find_element_in_scope(parser._html_elem_p, scope = parser._button_scope):
                    self._parse_error(location, "Invalid end tag: No <p> element to close (introduced)")
                    self._open_synthetic_element(parser._html_elem_p)
                self._close_a_p_element(location)
                return True
            elif name_index == parser._elem_name_li:
                if not self._find_element_in_scope(parser._html_elem_li, scope = parser._list_item_scope):
                    self._parse_error(location, "Invalid end tag: No <li> element to close (ignored)")
                    return True
                self._generate_implied_end_tags(exception = parser._html_elem_li)
                if self._open_elements[-1].get_type() != parser._html_elem_li:
                    self._parse_error(location, "Invalid end tag: Inside element that is not implicitely closeable")
                while True:
                    elem = self._close_element()
                    if elem.get_type() == parser._html_elem_li:
                        break
                return True
            elif name_index in [parser._elem_name_dt, parser._elem_name_dd]:
                elem_type = (Namespace.HTML, name_index)
                if not self._find_element_in_scope(elem_type):
                    self._parse_error(location, "Invalid end tag: No element to close (ignored)")
                    return True
                self._generate_implied_end_tags(exception = elem_type)
                if self._open_elements[-1].get_type() != elem_type:
                    self._parse_error(location, "Invalid end tag: Inside element that is not implicitely closeable")
                while True:
                    elem = self._close_element()
                    if elem.get_type() == elem_type:
                        break
                return True
            elif name_index in [parser._elem_name_h1, parser._elem_name_h2, parser._elem_name_h3,
                                parser._elem_name_h4, parser._elem_name_h5, parser._elem_name_h6]:
                header_elem_types = {parser._html_elem_h1, parser._html_elem_h2, parser._html_elem_h3,
                                     parser._html_elem_h4, parser._html_elem_h5, parser._html_elem_h6}
                if not self._find_satisfying_element_in_scope(lambda elem: elem.get_type() in header_elem_types):
                    self._parse_error(location, "Invalid end tag: No element to close (ignored)")
                    return True
                self._generate_implied_end_tags()
                elem_type = (Namespace.HTML, name_index)
                if self._open_elements[-1].get_type() != elem_type:
                    self._parse_error(location, "Invalid end tag: Inside element that is not implicitely closeable")
                while True:
                    elem = self._close_element()
                    if elem.get_type() in header_elem_types:
                        break
                return True
            elif name_index in [parser._elem_name_a, parser._elem_name_em, parser._elem_name_strong,
                                parser._elem_name_small, parser._elem_name_s, parser._elem_name_code,
                                parser._elem_name_i, parser._elem_name_b, parser._elem_name_u,
                                parser._elem_name_strike, parser._elem_name_big, parser._elem_name_font,
                                parser._elem_name_nobr, parser._elem_name_tt]:
                done = self._adoption_agency(location, name_index)
                if done:
                    return True
            elif name_index in [parser._elem_name_object, parser._elem_name_applet, parser._elem_name_marquee]:
                elem_type = (Namespace.HTML, name_index)
                if not self._find_element_in_scope(elem_type):
                    self._parse_error(location, "Invalid end tag: No element to close (ignored)")
                    return True
                self._generate_implied_end_tags()
                if self._open_elements[-1].get_type() != elem_type:
                    self._parse_error(location, "Invalid end tag: Inside element that is not implicitely closeable")
                while True:
                    elem = self._close_element()
                    if elem.get_type() == elem_type:
                        break
                self._drop_active_formatting_elements()
                return True
            elif name_index == parser._elem_name_br:
                self._parse_error(location, "Invalid end tag: No end tag exists for <br> element (treated as start "
                                  "tag)")
                self._reconstruct_active_formatting_elements()
                self._open_synthetic_element(parser._html_elem_br)
                self._close_element()
                self._frameset_ok = False
                return True
            elif name_index in [parser._elem_name_html, parser._elem_name_body]:
                if not self._find_element_in_scope(parser._html_elem_body):
                    self._parse_error(location, "Error 3")    
                    return True
                if any(elem.get_type() not in parser._implicitly_closeable for elem in self._open_elements):
                    self._parse_error(location, "Error 4")    
                self._insertion_mode = _InsertionMode.AFTER_BODY
                processed = name_index == parser._elem_name_body
                return processed
            for elem in reversed(self._open_elements):
                elem_type = (Namespace.HTML, name_index)
                if elem.get_type() == elem_type:
                    self._generate_implied_end_tags(exception = elem_type)
                    if self._open_elements[-1] != elem:
                        self._parse_error(location, "Error 5")    
                    while True:
                        elem_2 = self._close_element()
                        if elem_2 == elem:
                            break
                    return True
                if elem.get_type() in parser._special_elements:
                    self._parse_error(location, "Invalid end tag: No matching start tag")
                    return True
            assert False
        if isinstance(token, _Comment):
            self._insert_comment(token.data)
            return True
        if isinstance(token, _EndOfInput):
            # FIXME: If the stack of template insertion modes is not empty, then process the token using the rules for the "in template" insertion mode.                                                                                 
            for elem in reversed(self._open_elements):
                if elem.get_type() not in parser._implicitly_closeable:
                    self._parse_error(location, "Unclosed element") # FIXME: Should this instead point to the start tag? But that context is no longer available!?!?                             
            return True
        if isinstance(token, _Doctype):
            self._bad_doctype(location)
            return True
        assert False

    def _process_token_text(self, token):
        if isinstance(token, _Data):
            # FIXME: Find a way to get U+0000 NULL characters replaced by U+FFFD REPLACEMENT CHARACTER characters. The tokenizer (in text mode) is supposed to do it, but does not. Should a tokenizer wrapper be developed with appropriate modes?                                                       
            self._insert_text(token.data)
            return True
        if isinstance(token, _StartTag):
            raise RuntimeError("Not yet implemented")              
        if isinstance(token, _EndTag):
            name_index = token.elem_name_index
            if name_index == self._open_elements[-1].name_index:
                self._close_element()
                self._insertion_mode = self._orig_insertion_mode
                return True
            raise RuntimeError("Not yet implemented")              
        if isinstance(token, _EndOfInput):
            raise RuntimeError("Not yet implemented")              
        if isinstance(token, _Doctype):
            raise RuntimeError("Not yet implemented")              
        # FIXME: What if tokenizer does not respect this assertion? It probably does not      
        assert False

    def _process_token_in_template(self, token):
        raise RuntimeError("Not yet implemented")              

    def _process_token_after_body(self, token):
        location = token.location
        parser = self._parser
        if isinstance(token, _Data):
            whitespace = token.skip_leading_whitespace()
            if whitespace:
                self._reconstruct_active_formatting_elements()
                self._insert_text(whitespace)
            if not token.data:
                return True
        elif isinstance(token, _StartTag):
            name_index = token.elem_name_index
            if name_index == parser._elem_name_html:
                return self._process_token_in_body(token)
        elif isinstance(token, _EndTag):
            name_index = token.elem_name_index
            if name_index == parser._elem_name_html:
                # FIXME: If the parser was created as part of the HTML fragment parsing algorithm, this is a parse error; ignore the token. (fragment case)    
                self._insertion_mode = _InsertionMode.AFTER_AFTER_BODY
                return True
        elif isinstance(token, _Comment):
            self._insert_comment(token.data, parent = self._open_elements[0])
            return True
        elif isinstance(token, _EndOfInput):
            return True
        elif isinstance(token, _Doctype):
            self._bad_doctype(location)
            return True
        self._parse_error(location, "Invalid token after <body> element (body reopened)")
        self._insertion_mode = _InsertionMode.IN_BODY
        return False # Reprocess

    def _process_token_in_frameset(self, token):
        location = token.location
        parser = self._parser
        if isinstance(token, _Data):
            whitespace = token.skip_leading_whitespace()
            if whitespace:
                self._insert_text(whitespace)
            if not token.data:
                return True
        elif isinstance(token, _StartTag):
            name_index = token.elem_name_index
            if name_index == parser._elem_name_frameset:
                self._open_html_element_for_token(token)
                return True
            elif name_index == parser._elem_name_frame:
                self._open_html_element_for_token(token)
                self._close_element()
                if token.selfclosing:
                    self._selfclosing_flag_acknowledged = True
                return True
            elif name_index == parser._elem_name_noframes:
                return self._process_token_in_head(token)
            elif name_index == parser._elem_name_html:
                return self._process_token_in_body(token)
        elif isinstance(token, _EndTag):
            name_index = token.elem_name_index
            if name_index == parser._elem_name_frameset:
                # FIXME: If the current node is the root html element, then this is a parse error; ignore the token. (fragment case)    
                self._close_element()
                # FIXME: Is the closed element guaranteed to be a <frameset> element?    
                # FIXME: If parser was created for purpose of parsing a fragment, do not switch insertion mode. (fragment case)    
                if self._open_elements[-1].get_type() != parser._html_elem_frameset:
                    self._insertion_mode = _InsertionMode.AFTER_FRAMESET
                return True
        elif isinstance(token, _Comment):
            self._insert_comment(token.data, parent = self._document)
            return True
        elif isinstance(token, _EndOfInput):
            # FIXME: If only one element is open, then there is no parse error (fragment case)                
            self._parse_error(location, "Unclosed <frameset> element at end of input (force-closed)")
            return True
        elif isinstance(token, _Doctype):
            self._bad_doctype(location)
            return True
        self._parse_error(location, "Invalid token in <frameset> element (ignored)")
        return True

    def _process_token_after_frameset(self, token):
        location = token.location
        parser = self._parser
        if isinstance(token, _Data):
            whitespace = token.skip_leading_whitespace()
            if whitespace:
                self._insert_text(whitespace)
            if not token.data:
                return True
        elif isinstance(token, _EndOfInput):
            return True
        elif isinstance(token, _StartTag):
            name_index = token.elem_name_index
            if name_index == parser._elem_name_noframes:
                return self._process_token_in_head(token)
            elif name_index == parser._elem_name_html:
                return self._process_token_in_body(token)
        elif isinstance(token, _EndTag):
            name_index = token.elem_name_index
            if name_index == parser._elem_name_html:
                self._insertion_mode = _InsertionMode.AFTER_AFTER_FRAMESET
                return True
        elif isinstance(token, _Comment):
            self._insert_comment(token.data, parent = self._document)
            return True
        elif isinstance(token, _Doctype):
            self._bad_doctype(location)
            return True
        self._parse_error(location, "Invalid token after top-most <frameset> element (ignored)")
        return True

    def _process_token_after_after_body(self, token):
        location = token.location
        parser = self._parser
        if isinstance(token, _Data):
            whitespace = token.skip_leading_whitespace()
            if whitespace:
                self._reconstruct_active_formatting_elements()
                self._insert_text(whitespace)
            if not token.data:
                return True
        elif isinstance(token, _StartTag):
            name_index = token.elem_name_index
            if name_index == parser._elem_name_html:
                return self._process_token_in_body(token)
        elif isinstance(token, _Comment):
            self._insert_comment(token.data, parent = self._document)
            return True
        elif isinstance(token, _EndOfInput):
            return True
        elif isinstance(token, _Doctype):
            self._bad_doctype(location)
            return True
        self._parse_error(location, "Invalid token after <html> element (body reopened)")
        self._insertion_mode = _InsertionMode.IN_BODY
        return False # Reprocess

    def _process_token_after_after_frameset(self, token):
        location = token.location
        parser = self._parser
        if isinstance(token, _Data):
            whitespace = token.skip_leading_whitespace()
            if whitespace:
                self._reconstruct_active_formatting_elements()
                self._insert_text(whitespace)
            if not token.data:
                return True
        elif isinstance(token, _StartTag):
            name_index = token.elem_name_index
            if name_index == parser._elem_name_noframes:
                return self._process_token_in_head(token)
            elif name_index == parser._elem_name_html:
                return self._process_token_in_body(token)
        elif isinstance(token, _Comment):
            self._insert_comment(token.data, parent = self._document)
            return True
        elif isinstance(token, _EndOfInput):
            return True
        elif isinstance(token, _Doctype):
            self._bad_doctype(location)
            return True
        self._parse_error(location, "Invalid token after <html> element in frameset document (ignored)")
        return False

    def _process_foreign_token(self, token):
        self._log("FOREIGN")
        location = token.location
        parser = self._parser
        def inval_html_elem():
            self._parse_error(location, "Invalid HTML element: Nested inside foreign element (foreign elements "
                              "are not implicitely closeable)")
            while True:
                elem = self._open_elements[-1]
                if self._is_mathml_text_integration_point(elem) or self._is_html_integration_point(elem):
                    break
                if elem.namespace == Namespace.HTML:
                    break
                self._close_element()
            self._process_html_token(token)
        if isinstance(token, _Data):
            data = token.data.replace("\0", "\u0394")
            if _has_nonwhitespace(data):
                self._frameset_ok = False
            self._insert_text(data)
            return
        if isinstance(token, _StartTag):
            name_index = token.elem_name_index
            if name_index in [parser._elem_name_head, parser._elem_name_meta, parser._elem_name_body,
                              parser._elem_name_h1, parser._elem_name_h2, parser._elem_name_h3, parser._elem_name_h4,
                              parser._elem_name_h5, parser._elem_name_h6, parser._elem_name_p, parser._elem_name_hr,
                              parser._elem_name_pre, parser._elem_name_blockquote, parser._elem_name_ol,
                              parser._elem_name_ul, parser._elem_name_menu, parser._elem_name_li, parser._elem_name_dl,
                              parser._elem_name_dt, parser._elem_name_dd, parser._elem_name_div, parser._elem_name_em,
                              parser._elem_name_strong, parser._elem_name_small, parser._elem_name_s,
                              parser._elem_name_ruby, parser._elem_name_code, parser._elem_name_var,
                              parser._elem_name_sub, parser._elem_name_sup, parser._elem_name_i, parser._elem_name_b,
                              parser._elem_name_u, parser._elem_name_span, parser._elem_name_br, parser._elem_name_img,
                              parser._elem_name_embed, parser._elem_name_table, parser._elem_name_listing,
                              parser._elem_name_strike, parser._elem_name_big, parser._elem_name_center,
                              parser._elem_name_nobr, parser._elem_name_tt]:
                inval_html_elem()
                return
            if name_index == parser._elem_name_font:
                for attr in token.attributes:
                    if attr.local_name in ["color", "face", "size"]:
                        inval_html_elem()
                        return
            elem = self._get_adjusted_current_element()
            assert elem
            if elem.namespace == Namespace.MATHML:
                self._adjust_mathml_attributes(token)
            elif elem.namespace == Namespace.SVG:
                self._adjust_svg_element_name(token)
                self._adjust_svg_attributes(token)
            else:
                assert False
            self._adjust_foreign_attributes(token)
            self._open_element_for_token(token, elem.namespace)
            if token.selfclosing:
                self._close_element()
                self._selfclosing_flag_acknowledged = True
            return
        if isinstance(token, _EndTag):
            name_index = token.elem_name_index
            if name_index in [parser._elem_name_p, parser._elem_name_br]:
                inval_html_elem()
                return
            elif name_index == parser._elem_name_script:
                if self._open_elements[-1].get_type() == parser._svg_elem_script:
                    self._close_element()
                    return
            tag_name = self._element_names[name_index]
            assert self._open_elements
            level = len(self._open_elements) - 1
            elem = self._open_elements[level]
            assert elem.namespace != Namespace.HTML
            if elem.orig_name_index != name_index:
                self._parse_error(location, "Invalid end tag: Nested inside foreign element (foreign elements are not "
                                  "implicitely closeable)")
            while True:
                if level == 0:
                    raise RuntimeError("Not yet implemented")                                                         
                if elem.orig_name_index == name_index:
                    break
                level -= 1
                elem = self._open_elements[level]
                if elem.namespace == Namespace.HTML:
                    self._process_html_token(token)
                    return
            while True:
                elem_2 = self._close_element()
                if elem_2 == elem:
                    break
            return
        if isinstance(token, _Comment):
            self._insert_comment(token.data)
            return
        if isinstance(token, _Doctype):
            self._bad_doctype(location)
            return
        assert False

    def _stop_parsing(self):
        while self._open_elements:
            self._close_element()

    def _get_adjusted_current_element(self):
        # FIXME: The adjusted current node is the context element if the parser was created as part of the HTML fragment parsing algorithm and the stack of open elements has only one element in it (fragment case)    
        if self._open_elements:
            return self._open_elements[-1]
        return None

    def _is_mathml_text_integration_point(self, elem):
        parser = self._parser
        return elem.get_type() in [parser._mathml_elem_mi, parser._mathml_elem_mn, parser._mathml_elem_mo,
                                   parser._mathml_elem_mtext, parser._mathml_elem_ms]

    def _is_html_integration_point(self, elem):
        parser = self._parser
        elem_type = elem.get_type()
        if elem_type == parser._mathml_elem_annotation_xml:
            attr = elem.attribute_map.get("encoding")
            return attr and attr.value.lower() in ["text/html", "application/xhtml+xml"]
        return elem_type in [parser._svg_elem_desc, parser._svg_elem_title, parser._svg_elem_foreign_object]

    def _push_active_formatting_element(self, elem):
        n = 0
        for i, elem_2 in reversed(list(enumerate(self._active_formatting_elements))):
            is_marker = not elem_2
            if is_marker:
                break
            if elem_2.get_type() != elem.get_type():
                continue
            same_attributes = True
            if len(elem_2.attributes) != len(elem.attributes):
                same_attributes = False
            else:
                for attr in elem.attributes:
                    attr_2 = elem_2.attribute_map.get(attr.get_qualified_name())
                    same = (attr_2 and attr_2.namespace == attr.namespace and attr_2.local_name == attr.local_name and
                            attr_2.value == attr.value)
                    if not same:
                        same_attributes = False
                        break
            if not same_attributes:
                continue
            n += 1
            if n > 3:
                self._remove_active_formatting_element(i)
                break
        self._append_active_formatting_element(elem)

    # Performs the "clear the list of active formatting elements up to the last marker"
    # operation as defined by the HTML standard.
    def _drop_active_formatting_elements(self):
        while True:
            elem = self._active_formatting_elements.pop()
            is_marker = not elem
            if is_marker:
                break

    def _reconstruct_active_formatting_elements(self):
        # Find the earliest position such that all entries after it are neither markers nor
        # in the stack of open elements
        i = len(self._active_formatting_elements)
        while i > 0:
            j = i - 1
            elem = self._active_formatting_elements[j]
            is_marker = not elem
            if is_marker or elem.is_open:
                break
            i = j
        # Reopen all elements beyond the identified position
        for i, elem in enumerate(self._active_formatting_elements[i:]):
            assert not elem.is_open
            elem_2 = self._do_open_element(elem.get_type(), elem.orig_name_index, elem.attributes, elem.attribute_map)
            self._replace_active_formatting_element(i, elem_2)

    def _adoption_agency(self, location, name_index):
        elem = self._open_elements[-1]
        if elem.name_index == name_index and not elem.is_active_formatting_element: # FIXME: How can this ever happen?                                             
            self._close_element()
            return True
        parser = self._parser
        outer_loop_counter = 0
        while True:
            if self._logger:
                self._log("STACK OF OPEN LEMENTS: %s" % ", ".join([self._element_names[e.name_index].upper() for e in self._open_elements]))    
                self._log("ACTIVE FORMATTING ELEMENTS: %s" % ", ".join([self._element_names[e.name_index].upper() for e in self._active_formatting_elements]))    
            if outer_loop_counter == 8:
                return True
            outer_loop_counter += 1
            format_elem_index = None
            for i, elem in reversed(list(enumerate(self._active_formatting_elements))):
                is_marker = not elem
                if is_marker:
                    break
                if elem.name_index == name_index:
                    format_elem_index = i
                    break
            if format_elem_index is None:
                return False
            format_elem = self._active_formatting_elements[format_elem_index]
            if not format_elem.is_open:
                self._parse_error(location, "Error 6")
                self._remove_active_formatting_element(format_elem_index)
                return True
            if not self._is_element_in_scope(format_elem):
                self._parse_error(location, "Error 7")
                return True
            if format_elem != self._open_elements[-1]:
                self._parse_error(location, "Formatting element end tag inside open child")
            level = None
            for i, elem in reversed(list(enumerate(self._open_elements))):
                if elem == format_elem:
                    break
                if elem.get_type() in parser._special_elements:
                    level = i
            if level is None:
                while True:
                    elem = self._close_element()
                    if elem == format_elem:
                        break
                self._remove_active_formatting_element(format_elem_index)
                return True
            furthest_block_level = level
            new_format_elem_index = None
            inner_loop_counter = 0
            while True:
                level -= 1
                elem = self._open_elements[level]
                if elem == format_elem:
                    break
                inner_loop_counter += 1
                if inner_loop_counter > 3 and elem.is_active_formatting_element:
                    index = self._find_active_formatting_element(elem)
                    assert index is not None
                    self._remove_active_formatting_element(index)
                    if new_format_elem_index is not None:
                        assert new_format_elem_index > index
                        new_format_elem_index -= 1
                if not elem.is_active_formatting_element:
                    self._remove_open_element(level)
                    assert furthest_block_level > level
                    furthest_block_level -= 1
                    continue
                elem_2 = self._create_element(elem.get_type(), elem.orig_name_index,
                                              elem.attributes, elem.attribute_map)
                index = self._find_active_formatting_element(elem)
                assert index is not None
                self._replace_active_formatting_element(index, elem_2)
                self._replace_open_element(level, elem_2)
                if new_format_elem_index is None:
                    new_format_elem_index = index + 1
                child = self._open_elements[level + 1]
                self._insert_element(child, parent = elem_2)
            format_elem_level = level
            assert format_elem_level > 0 # Because a formatting element cannot be at root
            child = self._open_elements[format_elem_level + 1]
            common_ancestor = self._open_elements[format_elem_level - 1]
            self._insert_element(child, parent = common_ancestor)
            new_format_elem = self._create_element(format_elem.get_type(), format_elem.orig_name_index,
                                                   format_elem.attributes, format_elem.attribute_map)
            furthest_block = self._open_elements[furthest_block_level]
            self._move_children(furthest_block, new_format_elem)
            self._insert_element(new_format_elem, parent = furthest_block)
            if new_format_elem_index is None:
                new_format_elem_index = format_elem_index + 1
            self._remove_active_formatting_element(format_elem_index)
            assert new_format_elem_index > format_elem_index
            new_format_elem_index -= 1
            self._insert_active_formatting_element(new_format_elem_index, new_format_elem)
            new_format_elem_level = furthest_block_level + 1
            self._remove_open_element(format_elem_level)
            assert new_format_elem_level > format_elem_level
            new_format_elem_level -= 1
            self._insert_open_element(new_format_elem_level, new_format_elem)

    def _find_active_formatting_element(self, elem):
        for i, elem_2 in reversed(list(enumerate(self._active_formatting_elements))):
            if elem_2 == elem:
                return i
        return None

    def _append_active_formatting_element(self, elem):
        self._active_formatting_elements.append(elem)
        elem.is_active_formatting_element = True

    def _insert_active_formatting_element(self, index, elem):
        self._active_formatting_elements.insert(index, elem)
        elem.is_active_formatting_element = True

    def _remove_active_formatting_element(self, index):
        elem = self._active_formatting_elements[index]
        del self._active_formatting_elements[index]
        elem.is_active_formatting_element = False
        return elem

    def _replace_active_formatting_element(self, index, new_elem):
        elem = self._active_formatting_elements[index]
        self._active_formatting_elements[index] = new_elem
        elem.is_active_formatting_element = False
        new_elem.is_active_formatting_element = True
        return elem

    def _close_a_p_element(self, location):
        parser = self._parser
        self._generate_implied_end_tags(exception = parser._html_elem_p)
        if self._open_elements[-1].get_type() != parser._html_elem_p:
            self._parse_error(location, "Paragraph end tag inside open child")
        while True:
            elem = self._close_element()
            if elem.get_type() == parser._html_elem_p:
                break

    def _generate_implied_end_tags(self, exception = None):
        parser = self._parser
        white_list = {parser._html_elem_p, parser._html_elem_li, parser._html_elem_dt, parser._html_elem_dd,
                      parser._html_elem_rt, parser._html_elem_rp, parser._html_elem_optgroup,
                      parser._html_elem_option, parser._html_elem_rb, parser._html_elem_rtc}
        while True:
            elem = self._open_elements[-1]
            if elem.get_type() == exception:
                break
            if elem.get_type() not in white_list:
                break
            self._close_element()

    def _open_html_element_for_token(self, token):
        return self._open_element_for_token(token, Namespace.HTML)

    def _open_element_for_token(self, token, namespace):
        assert type(token) == _StartTag
        elem_type = (namespace, token.elem_name_index)
        return self._open_element(elem_type, token.orig_elem_name_index, token.attributes)

    def _open_synthetic_element(self, elem_type):
        orig_name_index = elem_type[1]
        attributes = []
        return self._open_element(elem_type, orig_name_index, attributes)

    # This function corresponds to the "insert a foreign element" operation as defined
    # by the HTML standard (with `onlyAddToElementStack` being false).
    def _open_element(self, elem_type, orig_name_index, attributes):
        attribute_map = {}
        for attr in attributes:
            assert attr.local_name not in attribute_map
            attribute_map[attr.local_name] = attr
        return self._do_open_element(elem_type, orig_name_index, attributes, attribute_map)

    def _do_open_element(self, elem_type, orig_name_index, attributes, attribute_map):
        elem = self._create_element(elem_type, orig_name_index, attributes, attribute_map)
        self._insert_element(elem)
        self._append_open_element(elem)
        return elem

    def _close_element(self):
        # FIXME: When the current node is removed from the stack of open elements, process internal resource links given the current node's node document.        
        return self._pop_open_element()

    def _append_open_element(self, elem):
        self._open_elements.append(elem)
        elem.is_open = True

    def _pop_open_element(self):
        elem = self._open_elements.pop()
        elem.is_open = False
        return elem

    def _insert_open_element(self, level, elem):
        self._open_elements.insert(level, elem)
        elem.is_open = True

    def _remove_open_element(self, level):
        elem = self._open_elements[level]
        del self._open_elements[level]
        elem.is_open = False
        return elem

    def _replace_open_element(self, level, new_elem):
        elem = self._open_elements[level]
        self._open_elements[level] = new_elem
        elem.is_open = False
        new_elem.is_open = True

    # This function corresponds to the "creating an element for the token" operation as
    # defined by the HTML standard.
    def _create_element(self, elem_type, orig_name_index, attributes, attribute_map):
        # FIXME: Expand functionality as described here: https://html.spec.whatwg.org/commit-snapshots/00b42be693bf53ef2990ccb4f4da9df22d1b3df8/#create-an-element-for-the-token    
        namespace, name_index = elem_type
        prefix = None
        local_name = self._element_names[name_index]
        attributes_2 = []
        for attr in attributes:
            attributes_2.append((attr.namespace, attr.prefix, attr.local_name, attr.value))
        app_node = self._callbacks.create_element(namespace, prefix, local_name, attributes_2)
        return _Element(namespace, name_index, orig_name_index, attributes, attribute_map, app_node)

    def _insert_doctype(self, name, public_id, system_id, parent = None):
        app_node = self._callbacks.create_doctype(name, public_id, system_id)
        # FIXME: Skip if effective parent is not a document node (but can this ever occur).        
        self._insert_app_node(app_node, parent)

    def _insert_text(self, data, parent = None):
        app_node = self._callbacks.create_text(data)
        # FIXME: Skip if effective parent is a document node (but can this ever occur).        
        self._insert_app_node(app_node, parent)

    def _insert_comment(self, data, parent = None):
        app_node = self._callbacks.create_comment(data)
        self._insert_app_node(app_node, parent)

    def _insert_element(self, elem, parent = None):
        if not elem.app_node:
            return
        self._insert_app_node(elem.app_node, parent)

    # If the parent node (`parent`) is specified, append the specified application node
    # (`app_node`) to the children of that parent node. Otherwise, insert the specified
    # application node at "the appropriate place for inserting a node" as defined by the
    # HTML standard.
    def _insert_app_node(self, app_node, parent = None):
        # FIXME: When parent is not specified, insert at "the appropriate place for inserting a node"                         
        parent_2 = parent
        if not parent_2:
            parent_2 = self._document
            if self._open_elements:
                parent_2 = self._open_elements[-1]
        if not parent_2.app_node:
            return
        self._callbacks.append_child(app_node, parent_2.app_node)

    def _move_children(self, old_parent, new_parent):
        if not old_parent.app_node or not new_parent.app_node:
            return
        self._callbacks.move_children(old_parent.app_node, new_parent.app_node)

    def _is_element_in_scope(self, elem, scope = None):
        elem = self._find_satisfying_element_in_scope(lambda elem_2: elem_2 == elem, scope = scope)
        return bool(elem)

    def _find_element_in_scope(self, elem_type, scope = None):
        return self._find_satisfying_element_in_scope(lambda elem: elem.get_type() == elem_type, scope = scope)

    def _find_satisfying_element_in_scope(self, predicate, scope = None):
        scope_2 = scope
        if scope_2 is None:
            scope_2 = self._parser._default_scope
        assert self._parser._html_elem_html in scope_2
        for elem in reversed(self._open_elements):
            if predicate(elem):
                return elem
            if elem.get_type() in scope_2:
                return False
        # We can never get here, because the scope includes the HTML element which will
        # always be found at the top of the stack of open elements (strangely, that
        # stack grows in the downwards direction)
        assert False

    def _adjust_mathml_attributes(self, token):
        assert type(token) == _StartTag
        for attr in token.attributes:
            if attr.local_name == "definitionurl":
                attr.local_name = "definitionURL"

    def _adjust_svg_element_name(self, token):
        # FIXME: Should parser._svg_element_map instead map element name indexes?    
        name = self._element_names[token.elem_name_index]
        entry = self._parser._svg_element_map.get(name)
        if entry:
            new_name = entry
            token.elem_name_index = self._element_names.intern(new_name)

    def _adjust_svg_attributes(self, token):
        assert type(token) == _StartTag
        parser = self._parser
        for attr in token.attributes:
            entry = parser._svg_attribute_map.get(attr.local_name)
            if not entry:
                continue
            attr.nocal_name = entry

    def _adjust_foreign_attributes(self, token):
        assert type(token) == _StartTag
        parser = self._parser
        for attr in token.attributes:
            entry = parser._foreign_attribute_map.get(attr.local_name)
            if not entry:
                continue
            (attr._namespace, attr.prefix, attr.local_name) = entry

    def _bad_doctype(self, location):
        self._parse_error(location, "Extraneous or misplaced document type declaration (ignored)")

    def _parse_error(self, location, message):
        self._error_handler.error(location, message)

    def _log(self, message, *params):
        if self._logger:
            self._logger.log(message % params)


# FIXME: Does tokenizer comply with required newline normalization (https://infra.spec.whatwg.org/#normalize-newlines)?    
class _Tokenizer(html.parser.HTMLParser):
    def __init__(self, session):
        html.parser.HTMLParser.__init__(self)
        self._session = session
    def handle_decl(self, decl):
        self._session.handle_doctype(decl)
    def handle_data(self, data):
        self._session.handle_data(data)
    def handle_starttag(self, tag, attrs):
        self._session.handle_starttag(tag, attrs)
    def handle_endtag(self, tag):
        self._session.handle_endtag(tag)
    def handle_startendtag(self, tag, attrs):
        self._session.handle_startendtag(tag, attrs)
    def handle_comment(self, data):
        self._session.handle_comment(data)


class _Parent:
    def __init__(self, app_node):
        self.app_node = app_node

class _Document(_Parent):
    def __init__(self, app_node):
        _Parent.__init__(self, app_node)

class _Attribute:
    def __init__(self, namespace, prefix, local_name, value):
        self.namespace = namespace
        self.prefix = prefix
        self.local_name = local_name
        self.value = value

    def get_qualified_name(self):
        if self.prefix is None:
            return self.local_name
        return "%s:%s" % (self.prefix, self.local_name)

# `attributes` is a list of objects of type `_Attribute`
# `attribute_map` maps a qualified name to a object in `attributes`
# INVARIANT: No two attributes have the same prefix and local name
# INVARIANT: No attribute has the same local name as the qualified name of another
# INVARIANT: Every attribute has a corresponding entry in `attribute_map`
class _Element(_Parent):
    def __init__(self, namespace, name_index, orig_name_index, attributes, attribute_map, app_node):
        _Parent.__init__(self, app_node)
        self.namespace = namespace
        self.name_index = name_index
        self.orig_name_index = orig_name_index
        self.attributes = attributes
        self.attribute_map = attribute_map
        self.is_open = False
        self.is_active_formatting_element = False
    def get_type(self):
        return (self.namespace, self.name_index)


class Token:
    def __init__(self, location):
        self.location = location

class _Doctype(Token):
    def __init__(self, location, name, public_id, system_id, force_quirks_mode):
        Token.__init__(self, location)
        self.name = name
        self.public_id = public_id
        self.system_id = system_id
        self.force_quirks_mode = force_quirks_mode

class _Data(Token):
    def __init__(self, location, data):
        Token.__init__(self, location)
        self.data = data

    def skip_leading_whitespace(self):
        location = self.location
        for i, ch in enumerate(self.data):
            if ch == "\n":
                location.line_number += 1
                location.position = 0
                continue
            # FIXME: According to 13.2.3.5 "Preprocessing the input stream", there are never any U+000D carriage returns in input to the tokenizer. But then why does the parser have specific rules for U+000D carriage returns?           
            if ch in "\t\f\r ":
                location.position += 1
                continue
            whitespace = self.data[:i]
            self.data = self.data[i:]
            return whitespace
        whitespace = self.data
        self.data = ""
        return whitespace

class _StartTag(Token):
    def __init__(self, location, elem_name_index, attributes, selfclosing):
        Token.__init__(self, location)
        self.elem_name_index = elem_name_index
        self.orig_elem_name_index = elem_name_index
        self.attributes = attributes
        self.selfclosing = selfclosing

class _EndTag(Token):
    def __init__(self, location, elem_name_index):
        Token.__init__(self, location)
        self.elem_name_index = elem_name_index

class _Comment(Token):
    def __init__(self, location, data):
        Token.__init__(self, location)
        self.data = data

class _EndOfInput(Token):
    def __init__(self, location):
        Token.__init__(self, location)


class Location:
    def __init__(self, line_number, position):
        self.line_number = line_number
        self.position = position


class _InsertionMode(enum.Enum):
    INITIAL              = enum.auto()
    BEFORE_HTML          = enum.auto()
    BEFORE_HEAD          = enum.auto()
    IN_HEAD              = enum.auto()
    IN_HEAD_NOSCRIPT     = enum.auto()
    AFTER_HEAD           = enum.auto()
    IN_BODY              = enum.auto()
    TEXT                 = enum.auto()
    IN_TEMPLATE          = enum.auto()
    AFTER_BODY           = enum.auto()
    IN_FRAMESET          = enum.auto()
    AFTER_FRAMESET       = enum.auto()
    AFTER_AFTER_BODY     = enum.auto()
    AFTER_AFTER_FRAMESET = enum.auto()


class _StringRegistry:
    class Basis:
        def __init__(self):
            self._strings = []
            self._string_indexes = {}
        def __len__(self):
            return len(self._strings)
        def __getitem__(self, index):
            return self._strings[index]
        def register(self, string):
            n = len(self._strings)
            self._strings.append(string)
            index = self._string_indexes.setdefault(string, n)
            assert index == n
            return index
        def find(self, string):
            return self._string_indexes.get(string)

    def __init__(self, basis):
        self._basis = basis
        self._strings = []
        self._string_indexes = {}
        self._offset = len(basis)

    def __getitem__(self, index):
        offset = self._offset
        if index < offset:
            return self._basis[index]
        return self._strings[index - offset]

    def intern(self, string):
        index = self._basis.find(string)
        if index is not None:
            assert index < self._offset
            return index
        n = self._offset + len(self._strings)
        index = self._string_indexes.setdefault(string, n)
        if index == n:
            self._strings.append(string)
        return index


def _extract_snippet(line, position, max_size):
    ellipsis = "..."
    ellipsis_size = len(ellipsis)
    min_max_size = 2 * ellipsis_size
    if max_size < min_max_size:
        max_size = min_max_size
    line_size = len(line)
    assert position <= line_size
    size = min(line_size, max_size)
    lead = size // 2
    if lead > position:
        lead = position
    else:
        discr = (size - lead) - (line_size - position)
        if discr > 0:
            lead += discr
    trail = size - lead
    a = position - lead
    b = position + trail
    if a > 0:
        assert a <= position - ellipsis_size
        a += ellipsis_size
    if b < line_size:
        assert b - position >= ellipsis_size
        b -= ellipsis_size
    context = ""
    if a > 0:
        context += ellipsis
    context += "".join(ch if ch.isprintable() else " " for ch in line[a:b])
    if b < line_size:
        context += ellipsis
    return context, lead


def _has_nonwhitespace(data):
    for ch in data:
        if ch not in "\t\n\f\r ":
            return True
    return False
