#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <map>
#include <locale>
#include <iostream>

#include "demangle.hpp"
#include "timer.hpp"
#include "random.hpp"
#include "wildcard.hpp"
#include <archon/core/unit_test.hpp>

using namespace archon;
using namespace archon::core;
using namespace archon::core::unit_test;


namespace {

void replace_char(std::string& str, char c, const std::string& replacement)
{
    for (std::size_t pos = str.find(c); pos != std::string::npos; pos = str.find(c, pos + 1))
        str.replace(pos, 1, replacement);
}


std::string xml_escape(const std::string& value)
{
    std::string value_2 = value;
    replace_char(value_2, '&', "&amp;");
    replace_char(value_2, '<', "&lt;");
    replace_char(value_2, '>', "&gt;");
    replace_char(value_2, '\'', "&apos;");
    replace_char(value_2, '\"', "&quot;");
    return value_2;
}


class XmlReporter: public Reporter {
public:
    XmlReporter(std::ostream& out):
        m_out{out}
    {
    }

    ~XmlReporter() noexcept override
    {
    }

    void begin(const TestContext& context) override
    {
        auto key = key_type(context.test_index, context.recurrence_index);
        m_tests.emplace(key, Test{});
    }

    void fail(const TestContext& context, const char* file_name, long line_number,
              const std::string& message) override
    {
        Failure f;
        f.file_name = file_name;
        f.line_number = line_number;
        f.message = message;
        auto key = key_type(context.test_index, context.recurrence_index);
        auto i = m_tests.find(key);
        i->second.failures.push_back(f);
    }

    void end(const TestContext& context, double elapsed_seconds) override
    {
        auto key = key_type(context.test_index, context.recurrence_index);
        auto i = m_tests.find(key);
        i->second.elapsed_seconds = elapsed_seconds;
    }

    void summary(const SharedContext& context, const Summary& results_summary) override
    {
        m_out << "<?xml version=\"1.0\"?>\n"
            "<unittest-results "
            "tests=\"" << results_summary.num_executed_tests << "\" "
            "failedtests=\"" << results_summary.num_failed_tests << "\" "
            "checks=\"" << results_summary.num_executed_checks << "\" "
            "failures=\"" << results_summary.num_failed_checks << "\" "
            "time=\"" << results_summary.elapsed_seconds << "\">\n";

        for (const auto& p: m_tests) {
            auto key = p.first;
            const Test& t = p.second;
            std::size_t test_index = key.first;
            int recurrence_index = key.second;
            const TestDetails& details = context.test_list.get_test_details(test_index);
            std::string test_name{details.test_name};
            if (context.num_recurrences > 1)
                test_name = test_name + "#" + util::to_string(recurrence_index + 1);                                     

            m_out << "  <test suite=\"" << xml_escape(details.suite_name) << "\" "
                "name=\"" << xml_escape(test_name) << "\" "
                "time=\"" << t.elapsed_seconds << "\"";
            if (t.failures.empty()) {
                m_out << "/>\n";
                continue;
            }
            m_out << ">\n";

            for (auto& i_2: t.failures) {
                std::string msg = xml_escape(i_2.message);
                m_out << "    <failure message=\"" << i_2.file_name << ""
                    "(" << i_2.line_number << ") : " << msg << "\"/>\n";
            }
            m_out << "  </test>\n";
        }
        m_out << "</unittest-results>\n";
    }

protected:
    struct Failure {
        const char* file_name;
        long line_number;
        std::string message;
    };

    struct Test {
        std::vector<Failure> failures;
        double elapsed_seconds = 0;
    };

    using key_type = std::pair<std::size_t, int>; // (test index, recurrence index)
    std::map<key_type, Test> m_tests;

    std::ostream& m_out;
};


class JUnitReporter: public XmlReporter
{
public:
    JUnitReporter(std::ostream& out):
        XmlReporter{out}
    {
    }

    ~JUnitReporter() noexcept override
    {
    }

    void summary(const SharedContext& context, const Summary& results_summary) override
    {
        m_out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

        m_out << "<testsuites>\n"
            "  <testsuite "
            "name=\"archon-tests\" "
            "tests=\"" << results_summary.num_executed_tests << "\" "
            "disabled=\"" << results_summary.num_excluded_tests << "\" "
            "failures=\"" << results_summary.num_failed_tests << "\" "
            "id=\"0\" "
            "time=\"" << results_summary.elapsed_seconds << "\">\n";

        for (const auto& p: m_tests) {
            auto key = p.first;
            const Test& t = p.second;
            std::size_t test_index = key.first;
            int recurrence_index = key.second;
            const TestDetails& details = context.test_list.get_test_details(test_index);
            std::string test_name{details.test_name};
            if (context.num_recurrences > 1)
                test_name = test_name + "#" + util::to_string(recurrence_index + 1);                                     

            m_out << "    <testcase name=\"" << xml_escape(test_name) << "\" "
                "status=\"" << (t.failures.size() > 0 ? "failed" : "passed") << "\" "
                "classname=\"" << xml_escape(test_name) << "\" "
                "time=\"" << t.elapsed_seconds << "\"";

            if (t.failures.empty()) {
                m_out << "/>\n";
            }
            else {
                m_out << ">\n";

                for (auto& i_2: t.failures) {
                    std::string msg = xml_escape(i_2.message);
                    m_out << "      <failure type=\"assertion failed\" "
                        "message=\"" << i_2.file_name << "(" << i_2.line_number << ") : "
                        "" << msg << "\"/>\n";
                }
                m_out << "    </testcase>\n";
            }
        }
        m_out << "  </testsuite>\n</testsuites>\n";
    }
};


class ManifoldReporter: public Reporter {
public:
    ManifoldReporter(std::initializer_list<Reporter*> subreporters)
    {
        for (Reporter* r: subreporters)
            m_subreporters.push_back(r);
    }

    void begin(const TestContext& context) override
    {
        for (Reporter* r: m_subreporters)
            r->begin(context);
    }

    void fail(const TestContext& context, const char* file_name, long line_number,
              const std::string& message) override
    {
        for (Reporter* r: m_subreporters)
            r->fail(context, file_name, line_number, message);
    }

    void end(const TestContext& context, double elapsed_seconds) override
    {
        for (Reporter* r: m_subreporters)
            r->end(context, elapsed_seconds);
    }

    void summary(const SharedContext& context, const Summary& results_summary) override
    {
        for (Reporter* r: m_subreporters)
            r->summary(context, results_summary);
    }

protected:
    std::vector<Reporter*> m_subreporters;
};


class WildcardFilter: public Filter {
public:
    WildcardFilter(const std::string& filter)
    {
        bool exclude = false;
        auto i = filter.begin(), end = filter.end();
        for (;;) {
            // Skip space
            while (i != end) {
                if (*i != ' ')
                    break;
                ++i;
            }

            // End of input?
            if (i == end)
                break;

            auto word_begin = i;

            // Find end of word
            while (i != end) {
                if (*i == ' ')
                    break;
                ++i;
            }

            auto word_end = i;
            std::size_t word_size = word_end - word_begin;
            if (word_size == 1 && *word_begin == '-') {
                exclude = true;
                continue;
            }

            std::string word{word_begin, word_end};
            auto& patterns = (exclude ? m_exclude : m_include);
            patterns.emplace_back(word);
        }

        // Include everything if no includes are specified.
        if (m_include.empty())
            m_include.emplace_back("*");
    }

    ~WildcardFilter() noexcept
    {
    }

    bool include(const TestDetails& details) override
    {
        const char* name_begin = details.test_name.data();
        const char* name_end = name_begin + details.test_name.size();

        // Say "no" if it matches an exclude pattern
        for (const WildcardPattern& pattern: m_exclude) {
            if (pattern.match(name_begin, name_end))
                return false;
        }

        // Say "yes" if it matches an include pattern
        for (const WildcardPattern& pattern: m_include) {
            if (pattern.match(name_begin, name_end))
                return true;
        }

        // Not included
        return false;
    }

private:
    std::vector<WildcardPattern> m_include, m_exclude;
};


class IntraTestLogger: private Logger::LevelThreshold, public Logger {
public:
    IntraTestLogger(Logger& base_logger, Level threshold):
        Logger::LevelThreshold{},
        Logger{static_cast<Logger::LevelThreshold&>(*this)},
        m_base_logger{base_logger},
        m_level_threshold{threshold}
    {
    }

    void do_log(Logger::Level level, std::string message) override final
    {
        Logger::do_log(m_base_logger, level, message); // Throws
    }

private:
    Logger& m_base_logger;
    const Level m_level_threshold;

    Level get() const noexcept override final
    {
        return m_level_threshold;
    }
};


} // unnamed namespace


namespace archon {
namespace core {
namespace unit_test {


class TestList::SharedContextImpl: public SharedContext {
public:
    Reporter& reporter;
    const bool abort_on_failure;
    const Logger::Level intra_test_log_level;
    Mutex mutex;
    struct Entry {
        const Test* test;
        std::size_t test_index;
        int recurrence_index;
    };
    std::vector<Entry> concur_tests, no_concur_tests;
    std::size_t next_concur_test = 0; // Index into `concur_tests`
    long num_failed_tests = 0;
    long long num_checks = 0;
    long long num_failed_checks = 0;
    int num_ended_threads = 0;
    int last_thread_to_end = -1;

    SharedContextImpl(const TestList& tests, int repetitions, int threads, Logger& l, Reporter& r,
                      bool aof, Logger::Level itll):
        SharedContext{tests, repetitions, threads, l},
        reporter{r},
        abort_on_failure{aof},
        intra_test_log_level{itll}
    {
    }
};


class TestList::ThreadContextImpl: public ThreadContext {
public:
    IntraTestLogger intra_test_logger;
    SharedContextImpl& shared_context;
    Mutex mutex;
    std::atomic<long long> num_checks;
    long long num_failed_checks;
    long num_failed_tests;
    bool errors_seen;

    ThreadContextImpl(SharedContextImpl& sc, int ti, Logger* attached_logger):
        ThreadContext{sc, ti, (attached_logger ? *attached_logger : sc.report_logger)},
        intra_test_logger{ThreadContext::report_logger, sc.intra_test_log_level},
        shared_context{sc}
    {
    }

    void run();
    void nonconcur_run();

    void run(SharedContextImpl::Entry, UniqueLock&);
    void finalize(UniqueLock&);

private:
    void clear_counters()
    {
        num_checks = 0;
        num_failed_checks = 0;
        num_failed_tests = 0;
    }
};


void TestList::add(RunFunc run_func, IsEnabledFunc is_enabled_func, bool allow_concur,
                   const char* suite, const std::string& name, const char* file, long line)
{
    Test test;
    test.run_func = run_func;
    test.is_enabled_func = is_enabled_func;
    test.allow_concur = allow_concur;
    test.details.suite_name = suite;
    test.details.test_name = name;
    test.details.file_name = file;
    test.details.line_number = line;
    m_tests.reserve(m_tests.size() + 1); // Throws
    m_test_storage.push_back(test);      // Throws
    m_tests.push_back(&m_test_storage.back());
}


bool TestList::run(Config config)
{
    if (config.num_repetitions < 0)
        throw std::runtime_error("Bad number of repetitions");
    if (config.num_threads < 1)
        throw std::runtime_error("Bad number of threads");

    StderrLogger fallback_logger;
    Logger& root_logger = (config.logger ? *config.logger : fallback_logger);
    ThreadSafeLogger shared_logger(root_logger);

    Reporter fallback_reporter;
    Reporter& reporter = (config.reporter ? *config.reporter : fallback_reporter);

    // Filter (second component is test index)
    std::vector<std::pair<const Test*, std::size_t>> included_tests;
    std::size_t num_enabled = 0, num_disabled = 0;
    for (std::size_t i = 0; i < m_tests.size(); ++i) {
        const Test* test = m_tests[i];
        if (!(*test->is_enabled_func)()) {
            ++num_disabled;
            continue;
        }
        ++num_enabled;
        if (config.filter && !config.filter->include(test->details))
            continue;
        included_tests.emplace_back(test, i);
    }

    // Repeat
    int num_threads = config.num_threads;
    std::vector<SharedContextImpl::Entry> concur_tests, no_concur_tests;
    std::size_t num_executed_tests = 0;
    for (int i = 0; i < config.num_repetitions; ++i) {
        for (auto p: included_tests) {
            SharedContextImpl::Entry entry;
            entry.test = p.first;
            entry.test_index = p.second;
            entry.recurrence_index = i;
            const Test& test = *entry.test;
            // In case only one test thread was asked for, we run all tests as
            // nonconcurrent tests to avoid reordering
            auto& tests = (test.allow_concur && num_threads > 1 ? concur_tests : no_concur_tests);
            tests.push_back(entry);
            if (num_executed_tests == std::numeric_limits<std::size_t>::max())
                throw std::runtime_error("Too many tests");
            ++num_executed_tests;
        }
    }

    // Don't start more threads than are needed
    {
        std::size_t max_threads = concur_tests.size();
        if (max_threads == 0 && !no_concur_tests.empty())
            max_threads = 1;
        if (max_threads < unsigned(num_threads))
            num_threads = int(max_threads);
    }

    // Shuffle
    if (config.shuffle) {
        Random random{random_int<unsigned long>()}; // Seed from slow global generator
        random.shuffle(concur_tests.begin(), concur_tests.end());
        random.shuffle(no_concur_tests.begin(), no_concur_tests.end());
    }

    // Execute
    SharedContextImpl shared_context(*this, config.num_repetitions, num_threads, shared_logger,
                                     reporter, config.abort_on_failure,
                                     config.intra_test_log_level);
    shared_context.concur_tests = std::move(concur_tests);
    shared_context.no_concur_tests = std::move(no_concur_tests);
    std::unique_ptr<std::unique_ptr<Logger>[]> loggers;
    loggers.reset(new std::unique_ptr<Logger>[num_threads]);
    if (num_threads != 1 || !config.per_thread_log_path.empty()) {
        std::unique_ptr<Logger> logger;
        std::ostringstream formatter;
        formatter.imbue(std::locale::classic());
        formatter << num_threads;
        int thread_digits = int(formatter.str().size());
        formatter.fill('0');
        if (config.per_thread_log_path.empty()) {
            for (int i = 0; i != num_threads; ++i) {
                formatter.str(std::string());
                formatter << "Thread[" << std::setw(thread_digits) << (i + 1) << "]: ";
                loggers[i].reset(new PrefixLogger{formatter.str(), shared_logger});
            }
        }
        else {
            const std::string& format = config.per_thread_log_path;
            auto j = format.rfind('%');
            if (j == std::string::npos)
                throw std::runtime_error("No '%' in per-thread log path");
            std::string a = format.substr(0, j), b = format.substr(j + 1);
            for (int i = 0; i != num_threads; ++i) {
                formatter.str(std::string());
                formatter << a << std::setw(thread_digits) << (i + 1) << b;
                std::string path = formatter.str();
                shared_logger.info("Logging to %1", path);
                loggers[i].reset(new FileLogger{path});
            }
        }
    }
    Timer timer;
    if (num_threads == 1) {
        ThreadContextImpl thread_context{shared_context, 0, loggers[0].get()};
        thread_context.run();
        thread_context.nonconcur_run();
    }
    else {
        std::unique_ptr<std::unique_ptr<ThreadContextImpl>[]> thread_contexts;
        thread_contexts.reset(new std::unique_ptr<ThreadContextImpl>[num_threads]);
        for (int i = 0; i < num_threads; ++i)
            thread_contexts[i].reset(new ThreadContextImpl{shared_context, i, loggers[i].get()});

        // First execute regular (concurrent) tests
        {
            auto thread = [&](int i) {
                {
                    std::ostringstream out;
                    out.imbue(std::locale::classic());
                    out << "test-thread-" << (i + 1);
                    Thread::set_name(out.str());
                }
                thread_contexts[i]->run();
            };
            std::unique_ptr<Thread[]> threads(new Thread[num_threads]);
            for (int i = 0; i < num_threads; ++i)
                threads[i].start([=] { thread(i); });
            for (int i = 0; i < num_threads; ++i)
                threads[i].join();
        }

        // Then execute nonconcurrent tests on main thread
        if (shared_context.last_thread_to_end != -1)
            thread_contexts[shared_context.last_thread_to_end]->nonconcur_run();
    }

    // Summarize
    Summary results_summary;
    results_summary.num_disabled_tests = long(num_disabled);
    results_summary.num_excluded_tests = long(num_enabled - included_tests.size());
    results_summary.num_included_tests = long(included_tests.size());
    results_summary.num_executed_tests = long(num_executed_tests);
    results_summary.num_failed_tests = shared_context.num_failed_tests;
    results_summary.num_executed_checks = shared_context.num_checks;
    results_summary.num_failed_checks = shared_context.num_failed_checks;
    results_summary.elapsed_seconds = timer.get_elapsed_time();
    reporter.summary(shared_context, results_summary);

    return shared_context.num_failed_tests == 0;
}


void TestList::ThreadContextImpl::run()
{
    clear_counters();

    UniqueLock lock{shared_context.mutex};
    shared_context.reporter.thread_begin(*this);

    // First run the tests that can safely run concurrently with other threads
    // and with itself.
    while (shared_context.next_concur_test < shared_context.concur_tests.size()) {
        auto entry = shared_context.concur_tests[shared_context.next_concur_test++];
        run(entry, lock);
    }

    // When only the last test thread is running, we can run the tests that
    // cannot safely run concurrently with other threads or with itself, but
    // this has to happen on the main thread (the one that calls
    // TestList::run()).
    if (!shared_context.no_concur_tests.empty()) {
        int num_remaining_threads = shared_context.num_threads - shared_context.num_ended_threads;
        if (num_remaining_threads == 1) {
            // Tell the main thread which thread context to use for executing
            // the nonconcurrent tests (nonconcur_run()).
            shared_context.last_thread_to_end = thread_index;
            return;
        }
    }

    ++shared_context.num_ended_threads;
    finalize(lock);
}


void TestList::ThreadContextImpl::nonconcur_run()
{
    clear_counters();

    UniqueLock lock{shared_context.mutex};

    for (auto entry: shared_context.no_concur_tests)
        run(entry, lock);

    finalize(lock);
}


void TestList::ThreadContextImpl::run(SharedContextImpl::Entry entry, UniqueLock& lock)
{
    const Test& test = *entry.test;
    TestContext test_context{*this, test.details, entry.test_index, entry.recurrence_index};
    shared_context.reporter.begin(test_context);
    lock.unlock();

    errors_seen = false;
    Timer timer;
    try {
        (*test.run_func)(test_context);
    }
    catch (std::exception& ex) {
        std::string message = "Unhandled exception " + get_type_name(ex) + ": " + ex.what();
        test_context.test_failed(message);
    }
    catch (...) {
        std::string message = "Unhandled exception of unknown type";
        test_context.test_failed(message);
    }
    double elapsed_time = timer.get_elapsed_time();
    if (errors_seen)
        ++num_failed_tests;

    lock.lock();
    shared_context.reporter.end(test_context, elapsed_time);
}


void TestList::ThreadContextImpl::finalize(UniqueLock&)
{
    shared_context.num_failed_tests += num_failed_tests;
    shared_context.num_checks += num_checks;
    shared_context.num_failed_checks += num_failed_checks;

    shared_context.reporter.thread_end(*this);
}


TestList& get_default_test_list()
{
    static TestList list;
    return list;
}


TestContext::TestContext(TestList::ThreadContextImpl& tc, const TestDetails& td,
                         std::size_t ti, int ri):
    thread_context{tc},
    test_details{td},
    test_index{ti},
    recurrence_index{ri},
    logger{tc.intra_test_logger},
    m_thread_context{tc}
{
}


void TestContext::check_succeeded()
{
    ++m_thread_context.num_checks;
}


ARCHON_NORETURN void TestContext::abort()
{
    const SharedContext& context = thread_context.shared_context;
    const char* format = (context.num_threads == 1 ? "Aborting due to failure" :
                          "Aborting due to failure in test thread %1");
    context.report_logger.info(format, m_thread_context.thread_index + 1);
    ::abort();
}


void TestContext::check_failed(const char* file, long line, const std::string& message)
{
    {
        LockGuard lock{m_thread_context.mutex};
        ++m_thread_context.num_checks;
        ++m_thread_context.num_failed_checks;
        m_thread_context.errors_seen = true;
    }
    {
        TestList::SharedContextImpl& shared = m_thread_context.shared_context;
        LockGuard lock{shared.mutex};
        shared.reporter.fail(*this, file, line, message);
        if (shared.abort_on_failure)
            abort();
    }
}


void TestContext::test_failed(const std::string& message)
{
    {
        LockGuard lock{m_thread_context.mutex};
        m_thread_context.errors_seen = true;
    }
    {
        TestList::SharedContextImpl& shared = m_thread_context.shared_context;
        LockGuard lock{shared.mutex};
        shared.reporter.fail(*this, test_details.file_name, test_details.line_number, message);
        if (shared.abort_on_failure)
            abort();
    }
}


void TestContext::cond_failed(const char* file, long line, const char* macro_name,
                              const char* cond_text)
{
    std::string msg = std::string(macro_name) + "(" + cond_text + ") failed";
    check_failed(file, line, msg);
}


void TestContext::compare_failed(const char* file, long line, const char* macro_name,
                                 const char* a_text, const char* b_text,
                                 const std::string& a_val, const std::string& b_val)
{
    std::string msg = std::string(macro_name) + "(" + a_text + ", " + b_text + ") "
        "failed with (" + a_val + ", " + b_val + ")";
    check_failed(file, line, msg);
}


void TestContext::inexact_compare_failed(const char* file, long line, const char* macro_name,
                                         const char* a_text, const char* b_text,
                                         const char* eps_text, long double a, long double b,
                                         long double eps)
{
    std::ostringstream out;
    out.precision(std::numeric_limits<long double>::digits10 + 1);
    out << macro_name << "(" << a_text << ", " << b_text << ", " << eps_text << ") "
        "failed with (" << a << ", " << b << ", " << eps << ")";
    check_failed(file, line, out.str());
}


void TestContext::throw_failed(const char* file, long line, const char* expr_text,
                               const char* exception_name)
{
    std::ostringstream out;
    out << "CHECK_THROW(" << expr_text << ", " << exception_name << ") failed: Did not throw";
    check_failed(file, line, out.str());
}


void TestContext::throw_ex_failed(const char* file, long line, const char* expr_text,
                                  const char* exception_name, const char* exception_cond_text)
{
    std::ostringstream out;
    out << "CHECK_THROW_EX(" << expr_text << ", " << exception_name << ", "
        "" << exception_cond_text << ") failed: Did not throw";
    check_failed(file, line, out.str());
}


void TestContext::throw_ex_cond_failed(const char* file, long line, const char* expr_text,
                                       const char* exception_name, const char* exception_cond_text)
{
    std::ostringstream out;
    out << "CHECK_THROW_EX(" << expr_text << ", " << exception_name << ", "
        "" << exception_cond_text << ") failed: Did throw, but condition failed";
    check_failed(file, line, out.str());
}


void TestContext::throw_any_failed(const char* file, long line, const char* expr_text)
{
    std::ostringstream out;
    out << "CHECK_THROW_ANY(" << expr_text << ") failed: Did not throw";
    check_failed(file, line, out.str());
}


void Reporter::thread_begin(const ThreadContext&)
{
}

void Reporter::begin(const TestContext&)
{
}

void Reporter::fail(const TestContext&, const char*, long, const std::string&)
{
}

void Reporter::end(const TestContext&, double)
{
}

void Reporter::thread_end(const ThreadContext&)
{
}

void Reporter::summary(const SharedContext&, const Summary&)
{
}


class PatternBasedFileOrder::State: public RefCountBase {
public:
    std::map<const void*, int> m_major_map; // Key is address of TestDetails object

    std::vector<WildcardPattern> m_patterns;

    State(const char** patterns_begin, const char** patterns_end)
    {
        for (const char** i = patterns_begin; i != patterns_end; ++i)
            m_patterns.emplace_back(*i);
    }

    ~State() noexcept
    {
    }

    int get_major(const TestDetails& details)
    {
        auto i = m_major_map.find(&details);
        if (i != m_major_map.end())
            return i->second;
        auto j = m_patterns.begin(), end = m_patterns.end();
        while (j != end && !j->match(details.file_name))
            ++j;
        int major = int(j - m_patterns.begin());
        m_major_map[&details] = major;
        return major;
    }
};

bool PatternBasedFileOrder::operator()(const TestDetails& a, const TestDetails& b)
{
    int major_a = m_wrap.m_state->get_major(a);
    int major_b = m_wrap.m_state->get_major(b);
    if (major_a < major_b)
        return true;
    if (major_a > major_b)
        return false;
    int i = std::strcmp(a.file_name, b.file_name);
    return i < 0;
}

PatternBasedFileOrder::Wrap::Wrap(const char** patterns_begin, const char** patterns_end):
    m_state{new State(patterns_begin, patterns_end)}
{
}

PatternBasedFileOrder::Wrap::~Wrap() noexcept
{
}

PatternBasedFileOrder::Wrap::Wrap(const Wrap& w):
    m_state{w.m_state}
{
}

PatternBasedFileOrder::Wrap& PatternBasedFileOrder::Wrap::operator=(const Wrap& w)
{
    m_state = w.m_state;
    return *this;
}


SimpleReporter::SimpleReporter(bool report_progress)
{
    m_report_progress = report_progress;
}

void SimpleReporter::begin(const TestContext& context)
{
    if (!m_report_progress)
        return;

    const TestDetails& details = context.test_details;
    Logger& logger = context.thread_context.report_logger;
    auto format = (context.thread_context.shared_context.num_recurrences == 1 ?
                   "%1:%2: Begin %3" : "%1:%2: Begin %3#%4");
    logger.info(format, details.file_name, details.line_number, details.test_name,
                context.recurrence_index + 1);
}

void SimpleReporter::fail(const TestContext& context, const char* file_name, long line_number,
                          const std::string& message)
{
    const TestDetails& details = context.test_details;
    Logger& logger = context.thread_context.report_logger;
    auto format = (context.thread_context.shared_context.num_recurrences == 1 ?
                   "%1:%2: ERROR in %3: %5" : "%1:%2: ERROR in %3#%4: %5");
    logger.info(format, file_name, line_number, details.test_name,
                context.recurrence_index + 1, message);
}

void SimpleReporter::thread_end(const ThreadContext& context)
{
    if (!m_report_progress)
        return;

    if (context.shared_context.num_threads > 1) {
        Logger& logger = context.report_logger;
        logger.info("End of thread");
    }
}

void SimpleReporter::summary(const SharedContext& context, const Summary& results_summary)
{
    Logger& logger = context.report_logger;
    if (results_summary.num_failed_tests == 0) {
        logger.info("Success: All %1 tests passed (%2 checks).",
                    results_summary.num_executed_tests,
                    results_summary.num_executed_checks);
    }
    else {
        logger.info("FAILURE: %1 out of %2 tests failed (%3 out of %4 checks failed).",
                    results_summary.num_failed_tests, results_summary.num_executed_tests,
                    results_summary.num_failed_checks, results_summary.num_executed_checks);
    }
    logger.info("Test time: %1", Timer::format(results_summary.elapsed_seconds));
    if (results_summary.num_excluded_tests >= 1) {
        auto format = (results_summary.num_excluded_tests == 1 ?
                       "Note: One test was excluded!" : "Note: %1 tests were excluded!");
        logger.info(format, results_summary.num_excluded_tests);
    }
}

std::unique_ptr<Reporter> create_junit_reporter(std::ostream& out)
{
    return std::make_unique<JUnitReporter>(out);
}

std::unique_ptr<Reporter> create_xml_reporter(std::ostream& out)
{
    return std::make_unique<XmlReporter>(out);
}

std::unique_ptr<Reporter> create_twofold_reporter(Reporter& subreporter_1, Reporter& subreporter_2)
{
    using list = std::initializer_list<Reporter*>;
    return std::make_unique<ManifoldReporter>(list{&subreporter_1, &subreporter_2});
}


std::unique_ptr<Filter> create_wildcard_filter(const std::string& filter)
{
    return std::make_unique<WildcardFilter>(filter);
}


} // namespace unit_test
} // namespace core
} // namespace archon
