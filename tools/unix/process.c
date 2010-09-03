/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License.
 *
 * The Original Code is librabbitmq.
 *
 * The Initial Developers of the Original Code are LShift Ltd, Cohesive
 * Financial Technologies LLC, and Rabbit Technologies Ltd.  Portions
 * created before 22-Nov-2008 00:00:00 GMT by LShift Ltd, Cohesive
 * Financial Technologies LLC, or Rabbit Technologies Ltd are Copyright
 * (C) 2007-2008 LShift Ltd, Cohesive Financial Technologies LLC, and
 * Rabbit Technologies Ltd.
 *
 * Portions created by LShift Ltd are Copyright (C) 2007-2010 LShift
 * Ltd. Portions created by Cohesive Financial Technologies LLC are
 * Copyright (C) 2007-2010 Cohesive Financial Technologies
 * LLC. Portions created by Rabbit Technologies Ltd are Copyright (C)
 * 2007-2010 Rabbit Technologies Ltd.
 *
 * Portions created by Tony Garnock-Jones are Copyright (C) 2009-2010
 * LShift Ltd and Tony Garnock-Jones.
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License Version 2 or later (the "GPL"), in
 * which case the provisions of the GPL are applicable instead of those
 * above. If you wish to allow use of your version of this file only
 * under the terms of the GPL, and not to allow others to use your
 * version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the
 * notice and other provisions required by the GPL. If you do not
 * delete the provisions above, a recipient may use your version of
 * this file under the terms of any one of the MPL or the GPL.
 *
 * ***** END LICENSE BLOCK *****
 */

#include <unistd.h>
#include <errno.h>
#include <spawn.h>
#include <sys/wait.h>

#include "common.h"
#include "process.h"

extern char **environ;

void pipeline(const char *const *argv, struct pipeline *pl)
{
	posix_spawn_file_actions_t file_acts;

	int pipefds[2];
	if (pipe(pipefds))
		die_errno(errno, "pipe");

	die_errno(posix_spawn_file_actions_init(&file_acts),
		  "posix_spawn_file_actions_init");
	die_errno(posix_spawn_file_actions_adddup2(&file_acts, pipefds[0], 0),
		  "posix_spawn_file_actions_adddup2");
	die_errno(posix_spawn_file_actions_addclose(&file_acts, pipefds[0]),
		  "posix_spawn_file_actions_addclose");
	die_errno(posix_spawn_file_actions_addclose(&file_acts, pipefds[1]),
		  "posix_spawn_file_actions_addclose");

	die_errno(posix_spawnp(&pl->pid, argv[0], &file_acts, NULL,
			       (char * const *)argv, environ),
		  "posix_spawnp: %s", argv[0]);

	die_errno(posix_spawn_file_actions_destroy(&file_acts),
		  "posix_spawn_file_actions_destroy");
	
	if (close(pipefds[0]))
		die_errno(errno, "close");

	pl->infd = pipefds[1];
}

int finish_pipeline(struct pipeline *pl)
{
	int status;

	if (close(pl->infd))
		die_errno(errno, "close");
	if (waitpid(pl->pid, &status, 0) < 0)
		die_errno(errno, "waitpid");
	return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
