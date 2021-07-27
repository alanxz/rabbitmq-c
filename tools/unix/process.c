// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#include <errno.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
#include "process.h"

extern char **environ;

void pipeline(const char *const *argv, struct pipeline *pl) {
  posix_spawn_file_actions_t file_acts;

  int pipefds[2];
  if (pipe(pipefds)) {
    die_errno(errno, "pipe");
  }

  die_errno(posix_spawn_file_actions_init(&file_acts),
            "posix_spawn_file_actions_init");
  die_errno(posix_spawn_file_actions_adddup2(&file_acts, pipefds[0], 0),
            "posix_spawn_file_actions_adddup2");
  die_errno(posix_spawn_file_actions_addclose(&file_acts, pipefds[0]),
            "posix_spawn_file_actions_addclose");
  die_errno(posix_spawn_file_actions_addclose(&file_acts, pipefds[1]),
            "posix_spawn_file_actions_addclose");

  die_errno(posix_spawnp(&pl->pid, argv[0], &file_acts, NULL,
                         (char *const *)argv, environ),
            "posix_spawnp: %s", argv[0]);

  die_errno(posix_spawn_file_actions_destroy(&file_acts),
            "posix_spawn_file_actions_destroy");

  if (close(pipefds[0])) {
    die_errno(errno, "close");
  }

  pl->infd = pipefds[1];
}

int finish_pipeline(struct pipeline *pl) {
  int status;

  if (close(pl->infd)) {
    die_errno(errno, "close");
  }
  if (waitpid(pl->pid, &status, 0) < 0) {
    die_errno(errno, "waitpid");
  }
  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
