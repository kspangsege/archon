#include <locale>

#include <archon/base/enum.hpp>
#include <archon/base/prefix_logger.hpp>
#include <archon/version.hpp>


using namespace archon;
using namespace archon::base;


namespace {

enum class Color { orange, purple, brown };

} // unnamed namespace

namespace archon::base {

template<> struct EnumTraits<Color> {
    static constexpr bool is_specialized = true;
    struct Spec { static EnumAssoc map[]; };
    static constexpr bool ignore_case = false;
};

} // namespace archon::base

inline EnumAssoc archon::base::EnumTraits<Color>::Spec::map[] = {
    { int(Color::orange), "orange" },
    { int(Color::purple), "purple" },
    { int(Color::brown),  "brown"  }
};


int main()
{
    std::locale::global(std::locale(""));

    Color color = Color::purple;
    Logger root_logger;
    PrefixLogger foo_logger(root_logger, "Foo: ");
    PrefixLogger bar_logger(foo_logger, "Bar: ");
    PrefixLogger logger(bar_logger, "Gaz: ");
    logger.error("Hello (%s)\n\nKristian %s\n", color, ARCHON_VERSION);
}
