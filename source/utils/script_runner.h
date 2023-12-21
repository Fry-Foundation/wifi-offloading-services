#ifndef SCRIPT_RUNNER_H
#define SCRIPT_RUNNER_H

void runScriptAndSaveOutput(const char *scriptPath, const char *outputPath);

char* runScript(const char* scriptPath);

#endif // SCRIPT_RUNNER_H