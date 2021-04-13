#include "../../lib/buffer.h"
#include "../job.h"

/* ----------------------------------------------------------------------- */
void
job_print(struct job* job, buffer* out) {
  buffer_putc(out, '[');
  buffer_putulong(out, job->id);
  buffer_putc(out, ']');
  buffer_putc(out, ' ');
  buffer_puts(out, "  ");
  buffer_putspad(out, job->stopped ? "Stopped" : "Running", 24);
  buffer_puts(out, job->command ? job->command : "(null)");
  buffer_putnlflush(out);
}
