#ifndef SCRIPT_RUNNER_H
#define SCRIPT_RUNNER_H

void run_script_and_save_output(const char *script_path, const char *output_path);

char* run_script(const char* script_path);

#endif // SCRIPT_RUNNER_H