#pragma once

// Checks for the core tests: failures are counted and printed, never thrown.

#include <Project64-core/Notification.h>
#include <filesystem>
#include <sstream>
#include <stdio.h>
#include <string>

void TestBegin(const char * Name);
int TestEnd(void);
void TestFail(const char * File, int Line, const std::string & Message);

#define CHECK(Expr)                                    \
    do                                                 \
    {                                                  \
        if (!(Expr))                                   \
        {                                              \
            TestFail(__FILE__, __LINE__, "CHECK(" #Expr ")"); \
        }                                              \
    } while (0)

#define CHECK_EQ(Actual, Expected)                                               \
    do                                                                           \
    {                                                                            \
        auto TestActual = (Actual);                                              \
        auto TestExpected = (Expected);                                          \
        if (!(TestActual == TestExpected))                                       \
        {                                                                        \
            std::ostringstream TestMessage;                                      \
            TestMessage << #Actual << " = \"" << TestActual << "\", expected \"" \
                        << TestExpected << "\"";                                 \
            TestFail(__FILE__, __LINE__, TestMessage.str());                     \
        }                                                                        \
    } while (0)

// Messages go to stderr; questions are answered "no"
class TestNotification :
    public CNotification
{
public:
    void DisplayError(const char * Message) const override;
    void DisplayError(LanguageStringID StringID) const override;
    void FatalError(const char * Message) const override;
    void FatalError(LanguageStringID StringID) const override;
    void DisplayWarning(const char * Message) const override;
    void DisplayWarning(LanguageStringID StringID) const override;
    void DisplayMessage(int DisplayTime, const char * Message) const override;
    void DisplayMessage(int DisplayTime, LanguageStringID StringID) const override;
    void DisplayMessage2(const char * Message) const override;
    bool AskYesNoQuestion(const char * Question) const override;
    void BreakPoint(const char * FileName, int32_t LineNumber) override;
    void AppInitDone(void) override;
    bool ProcessGuiMessages(void) const override;
    void ChangeFullScreen(void) const override;
};

int TestGameKeys(const std::filesystem::path & WorkDir);
int TestBaseDir(const std::filesystem::path & WorkDir);
