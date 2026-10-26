// Core tests: Project64-test <test> <work directory>

#include "Test.h"

#include <Project64-core/Multilanguage.h>
#include <string.h>

namespace
{
const char * g_TestName = "";
int g_Failures = 0;
} // namespace

void TestBegin(const char * Name)
{
    g_TestName = Name;
    g_Failures = 0;
}

int TestEnd(void)
{
    fprintf(stderr, "%s: %s (%d failure%s)\n", g_TestName, g_Failures == 0 ? "ok" : "FAILED", g_Failures, g_Failures == 1 ? "" : "s");
    return g_Failures == 0 ? 0 : 1;
}

void TestFail(const char * File, int Line, const std::string & Message)
{
    fprintf(stderr, "%s:%d: %s\n", File, Line, Message.c_str());
    g_Failures++;
}

void TestNotification::DisplayError(const char * Message) const
{
    fprintf(stderr, "error: %s\n", Message);
}

void TestNotification::DisplayError(LanguageStringID StringID) const
{
    DisplayError(GS(StringID));
}

void TestNotification::FatalError(const char * Message) const
{
    fprintf(stderr, "fatal: %s\n", Message);
}

void TestNotification::FatalError(LanguageStringID StringID) const
{
    FatalError(GS(StringID));
}

void TestNotification::DisplayWarning(const char * Message) const
{
    fprintf(stderr, "warning: %s\n", Message);
}

void TestNotification::DisplayWarning(LanguageStringID StringID) const
{
    DisplayWarning(GS(StringID));
}

void TestNotification::DisplayMessage(int /*DisplayTime*/, const char * /*Message*/) const
{
}

void TestNotification::DisplayMessage(int /*DisplayTime*/, LanguageStringID /*StringID*/) const
{
}

void TestNotification::DisplayMessage2(const char * /*Message*/) const
{
}

bool TestNotification::AskYesNoQuestion(const char * Question) const
{
    fprintf(stderr, "question (answered no): %s\n", Question);
    return false;
}

void TestNotification::BreakPoint(const char * FileName, int32_t LineNumber)
{
    TestFail(FileName, LineNumber, "core breakpoint");
}

void TestNotification::AppInitDone(void)
{
}

bool TestNotification::ProcessGuiMessages(void) const
{
    return false;
}

void TestNotification::ChangeFullScreen(void) const
{
}

int main(int argc, char ** argv)
{
    if (argc == 3 && strcmp(argv[1], "game-keys") == 0)
    {
        return TestGameKeys(argv[2]);
    }
    if (argc == 3 && strcmp(argv[1], "base-dir") == 0)
    {
        return TestBaseDir(argv[2]);
    }
    fprintf(stderr, "usage: %s game-keys|base-dir <work directory>\n", argv[0]);
    return 2;
}
