#ifndef AICS_LOGGER_H
#define AICS_LOGGER_H

#include <stdio.h>

namespace aics {
namespace agentsim {

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define PRINT_INFO(pargs) printf("[ INFO ] " pargs);
#define PRINT_ERROR(pargs) printf(KRED "[ ERROR ] " pargs KNRM);
#define PRINT_WARNING(pargs) printf(KYEL "[ WARNING ] " pargs KNRM);

} // namespace agentsim
} // namespace aics

#endif // AICS_LOGGER_H
