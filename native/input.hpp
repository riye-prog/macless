#pragma once
#include <string>
#include <vector>

namespace toolscreen {
int scanFromCode(int keycode);
std::string scanName(int scan);
std::vector<int> modifierScans(int modifiers);
int inputEvent(int scan,int modifiers,int action,bool gameplay,bool editorAllowed);
void releasePhysicalInput(bool restore=true);
int inputPersistentMode(int current);
}
