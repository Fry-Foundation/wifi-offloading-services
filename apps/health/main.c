#include "core/console.h"

static Console csl = {
    .topic = "main",
};

int main(int argc, char *argv[]) {
    console_info(&csl, "Health service started");
}
