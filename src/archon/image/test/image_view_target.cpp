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

#include <archon/core/text.hpp>
#include <archon/core/options.hpp>
#include <archon/core/file.hpp>
#include <archon/image/oper.hpp>
#include <archon/image/imageio.hpp>


using namespace std;
using namespace archon::Core;
using namespace archon::Imaging;

namespace
{
  string opt_view = "ident";
  string opt_image = "";


  bool check_view(string name, string opt, vector<int> &params)
  {
    if(!Text::is_prefix(name, opt)) return false;
    if(name.size() == opt.size()) return true;
    if(opt[name.size()] != ',') throw invalid_argument("Syntax error in view params");
    istringstream i(opt.substr(name.size()+1));
    Text::SimpleTokenizer<char> tokenizer(i, ",", Text::SimpleTokenizer<char>::incl_empty);
    string s;
    while(tokenizer.generate(s)) params.push_back(Text::parse<int>(s));
    if(params.empty()) throw invalid_argument("Syntax error in view params");
    return true;
  }
}


int main(int argc, char const *argv[]) throw()
{
  CommandlineOptions opts;
  opts.add_help("Test application for image operator compositions as targets");
  opts.check_num_args();
  opts.add_param("v", "view", opt_view,
                 "Choose from:\n"
                 "ident\n"
//                 "cut,left,bottom,width,height[,horizontalrepeat,verticalrepeat]\n"
                 "flip,horizontal,vertical\n"
                 "diagflip,even,odd\n"
                 "rot,ninety,oneeighty\n"
                 "invert[,index]\n"
                 "channel,index[,preserve-alpha]");
  opts.add_param("i", "image", opt_image, "Image to be written through the selected view");
  if(int stop = opts.process(argc, argv)) return stop == 2 ? 0 : 1;

  if(opt_image.empty()) opt_image = File::dir_of(argv[0])+"../alley_baggett.png";

  Image::Ref image = ImageIO::load(*make_stdin_stream(), "stream:in:std");
  Image::Ref view;

  vector<int> params;
  if(check_view("ident", opt_view, params))
  {
    if(0 < params.size()) throw invalid_argument("Wrong number of view params");
    view = image;
  }
/*
  else if(check_view("cut", opt_view, params))
  {
    if(params.size() != 4 && params.size() != 6) throw invalid_argument("Wrong number of view params");
    if(params.size() == 4) view = image->getSubView(params[0], params[1], params[2], params[3]);
    else view = image->getSubView(params[0], params[1], params[2], params[3], params[4], params[5]);
  }
*/
  else if(check_view("flip", opt_view, params))
  {
    if(params.size() != 2) throw invalid_argument("Wrong number of view params");
    view = Oper::flip(image, params[0], params[1]);
  }
  else if(check_view("diagflip", opt_view, params))
  {
    if(params.size() != 2) throw invalid_argument("Wrong number of view params");
    view = Oper::flip_diag(image, params[0], params[1]);
  }
  else if(check_view("rot", opt_view, params))
  {
    if(params.size() != 2) throw invalid_argument("Wrong number of view params");
    view = Oper::rotate(image, params[0], params[1]);
  }
  else if(check_view("invert", opt_view, params))
  {
    if(params.size() != 0 && params.size() != 1) throw invalid_argument("Wrong number of view params");
    view = params.size() == 0 ? Oper::invert(image) : Oper::invert(image, params[0]);
  }
  else if(check_view("channel", opt_view, params))
  {
    if(params.size() != 1 && params.size() != 2) throw invalid_argument("Wrong number of view params");
    view = params.size() == 1 ? Oper::pick_channel(image, params[0]) :
      Oper::pick_channel(image, params[0], params[1]);
  }
  else throw invalid_argument("Unknown view '"+opt_view+"'");

  view->put_image(Image::load(opt_image));

  ImageIO::save(image, *make_stdout_stream(), "stream:out:std", "png");

  return 0;
}
