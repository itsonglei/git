#ifndef RUN_COMMAND_H
#define RUN_COMMAND_H

#include "thread-utils.h"

#include "argv-array.h"

/**
 * The run-command API offers a versatile tool to run sub-processes with
 * redirected input and output as well as with a modified environment
 * and an alternate current directory.
 * run-command
 * API提供了一个通用的工具，可以使用重定向的输入和输出以及修改的环境和备用当前目录来运行子进程。
 *
 * A similar API offers the capability to run a function asynchronously,
 * which is primarily used to capture the output that the function
 * produces in the caller in order to process it.
 * 类似的API提供异步运行函数的功能，主要用于捕获函数在调用者中生成的输出，以便处理它。
 */

/**
 * This describes the arguments, redirections, and environment of a
 * command to run in a sub-process.
 * 它描述要在子进程中运行的命令的参数、重定向和环境。
 *
 * The caller:
 * 调用者:
 *
 * 1. allocates and clears (using child_process_init() or
 *    CHILD_PROCESS_INIT) a struct child_process variable;
 * 2. initializes the members;
 * 3. calls start_command();
 * 4. processes the data;
 * 5. closes file descriptors (if necessary; see below);
 * 6. calls finish_command().
 * 1. 分配并清除一个struct
 *child_process变量(使用child_process_init()或child_process_init);
 * 2。初始化成员;
 * 3。调用start_command ();
 * 4。处理数据;
 * 5。关闭文件描述符(如有需要;见下文);
 * 6。调用finish_command ()。
 *
 * Special forms of redirection are available by setting these members
 * to 1:
 * 特殊形式的重定向可通过设置这些成员为1:
 *
 *  .no_stdin, .no_stdout, .no_stderr: The respective channel is
 *		redirected to /dev/null.
 *
 *	.stdout_to_stderr: stdout of the child is redirected to its
 *		stderr. This happens after stderr is itself redirected.
 *		So stdout will follow stderr to wherever it is
 *		redirected.
 */
struct child_process {
	/**
	 * The .argv member is set up as an array of string pointers (NULL
	 * terminated), of which .argv[0] is the program name to run (usually
	 * without a path). If the command to run is a git command, set argv[0]
	 * to the command name without the 'git-' prefix and set .git_cmd = 1.
	 *
	 * argv成员被设置为一个字符串指针数组(以NULL结尾)，其中.argv[0]是要运行的程序名(通常没有路径)。如果要运行的命令是一个git命令，那么将argv[0]设置为不带“git-”前缀的命令名，并设置.git_cmd
	 * = 1。
	 *
	 * Note that the ownership of the memory pointed to by .argv stays with
	 * the caller, but it should survive until `finish_command` completes.
	 * If the .argv member is NULL, `start_command` will point it at the
	 * .args `argv_array` (so you may use one or the other, but you must use
	 * exactly one). The memory in .args will be cleaned up automatically
	 * during `finish_command` (or during `start_command` when it is
	 * unsuccessful).
	 *
	 * 请注意，.argv指向的内存的所有权保留在调用者那里，但是它应该一直存在，直到'
	 * finish_command '完成。如果.argv成员为NULL， ' start_command
	 * '将把它指向.args ' argv_array
	 * '(所以你可以使用一个或另一个，但你必须使用一个)。.args中的内存将在“finish_command”期间(或在“start_command”不成功时)被自动清除。
	 *
	 */
	const char **argv;

	struct argv_array args;
	struct argv_array env_array;
	pid_t pid;

	int trace2_child_id;
	uint64_t trace2_child_us_start;
	const char *trace2_child_class;
	const char *trace2_hook_name;

	/*
	 * Using .in, .out, .err:
	 * - Specify 0 for no redirections. No new file descriptor is allocated.
	 * (child inherits stdin, stdout, stderr from parent).
	 * - Specify -1 to have a pipe allocated as follows:
	 *     .in: returns the writable pipe end; parent writes to it,
	 *          the readable pipe end becomes child's stdin
	 *     .out, .err: returns the readable pipe end; parent reads from
	 *          it, the writable pipe end becomes child's stdout/stderr
	 *   The caller of start_command() must close the returned FDs
	 *   after it has completed reading from/writing to it!
	 * - Specify > 0 to set a channel to a particular FD as follows:
	 *     .in: a readable FD, becomes child's stdin
	 *     .out: a writable FD, becomes child's stdout/stderr
	 *     .err: a writable FD, becomes child's stderr
	 *   The specified FD is closed by start_command(), even in case
	 *   of errors!
	 */
	int in;
	int out;
	int err;

	/**
	 * To specify a new initial working directory for the sub-process,
	 * specify it in the .dir member.
	 * 要为子流程指定新的初始工作目录，请在.dir成员中指定它。
	 */
	const char *dir;

	/**
	 * To modify the environment of the sub-process, specify an array of
	 * string pointers (NULL terminated) in .env:
	 * 要修改子进程的环境，请在.env中指定一个字符串指针数组(以NULL结尾):
	 *
	 * - If the string is of the form "VAR=value", i.e. it contains '='
	 *   the variable is added to the child process's environment.
	 *
	 * - If the string does not contain '=', it names an environment
	 *   variable that will be removed from the child process's environment.
	 *
	 * If the .env member is NULL, `start_command` will point it at the
	 * .env_array `argv_array` (so you may use one or the other, but not
	 * both). The memory in .env_array will be cleaned up automatically
	 * during `finish_command` (or during `start_command` when it is
	 * unsuccessful).
	 * 如果.env成员为NULL， ' start_command '将把它指向.env_array '
	 * argv_array
	 * '(因此可以使用其中一个，但不能同时使用两个)。env_array中的内存将在“finish_command”期间自动清除(或在“start_command”不成功时清除)。
	 */
	const char *const *env;

	unsigned no_stdin : 1;
	unsigned no_stdout : 1;
	unsigned no_stderr : 1;
	unsigned git_cmd : 1; /* if this is to be git sub-command */

	/**
	 * If the program cannot be found, the functions return -1 and set
	 * errno to ENOENT. Normally, an error message is printed, but if
	 * .silent_exec_failure is set to 1, no message is printed for this
	 * special error condition.
	 * 如果找不到程序，函数返回-1并将errno设置为ENOENT。通常，会打印一条错误消息，但是如果.silent_exec_failure设置为1，则不会为此特殊错误条件打印任何消息。
	 */
	unsigned silent_exec_failure : 1;

	unsigned stdout_to_stderr : 1;
	unsigned use_shell : 1;
	unsigned clean_on_exit : 1;
	unsigned wait_after_clean : 1;
	void (*clean_on_exit_handler)(struct child_process *process);
	void *clean_on_exit_handler_cbdata;
};

#define CHILD_PROCESS_INIT                             \
	{                                              \
		NULL, ARGV_ARRAY_INIT, ARGV_ARRAY_INIT \
	}

/**
 * The functions: child_process_init, start_command, finish_command,
 * run_command, run_command_v_opt, run_command_v_opt_cd_env, child_process_clear
 * do the following:
 *
 * - If a system call failed, errno is set and -1 is returned. A diagnostic
 *   is printed.
 *
 * - If the program was not found, then -1 is returned and errno is set to
 *   ENOENT; a diagnostic is printed only if .silent_exec_failure is 0.
 *
 * - Otherwise, the program is run. If it terminates regularly, its exit
 *   code is returned. No diagnostic is printed, even if the exit code is
 *   non-zero.
 *
 * - If the program terminated due to a signal, then the return value is the
 *   signal number + 128, ie. the same value that a POSIX shell's $? would
 *   report.  A diagnostic is printed.
 *
 */

/**
 * Initialize a struct child_process variable.
 * 初始化struct child_process变量。
 */
void child_process_init(struct child_process *);

/**
 * Release the memory associated with the struct child_process.
 * Most users of the run-command API don't need to call this
 * function explicitly because `start_command` invokes it on
 * failure and `finish_command` calls it automatically already.
 * 释放与struct child_process关联的内存。run-command
 * API的大多数用户不需要显式地调用这个函数，因为“start_command”在出现故障时调用它，而“finish_command”已经自动调用它了。
 */
void child_process_clear(struct child_process *);

int is_executable(const char *name);

/**
 * Start a sub-process. Takes a pointer to a `struct child_process`
 * that specifies the details and returns pipe FDs (if requested).
 * See below for details.
 * 启动子流程。获取指向' struct child_process
 * '的指针，该指针指定细节并返回管道FDs(如果被请求)。详见下文。
 */
int start_command(struct child_process *);

/**
 * Wait for the completion of a sub-process that was started with
 * start_command().
 * 等待用start_command()启动的子进程完成。
 */
int finish_command(struct child_process *);

int finish_command_in_signal(struct child_process *);

/**
 * A convenience function that encapsulates a sequence of
 * start_command() followed by finish_command(). Takes a pointer
 * to a `struct child_process` that specifies the details.
 * 一个方便的函数，它封装了start_command()和finish_command()的序列。获取指向指定细节的'
 * struct child_process '的指针。
 */
int run_command(struct child_process *);

/*
 * Returns the path to the hook file, or NULL if the hook is missing
 * or disabled. Note that this points to static storage that will be
 * overwritten by further calls to find_hook and run_hook_*.
 * 返回钩子文件的路径，如果钩子丢失或禁用，则返回NULL。注意，它指向的静态存储将被对find_hook和run_hook_*的进一步调用覆盖。
 */
const char *find_hook(const char *name);

/**
 * Run a hook.
 * The first argument is a pathname to an index file, or NULL
 * if the hook uses the default index file or no index is needed.
 * The second argument is the name of the hook.
 * The further arguments correspond to the hook arguments.
 * The last argument has to be NULL to terminate the arguments list.
 * If the hook does not exist or is not executable, the return
 * value will be zero.
 * If it is executable, the hook will be executed and the exit
 * status of the hook is returned.
 * On execution, .stdout_to_stderr and .no_stdin will be set.
 * 第一个参数是索引文件的路径名，如果钩子使用默认索引文件或不需要索引，则为NULL。第二个参数是钩子的名称。进一步的参数对应于钩子参数。最后一个参数必须为NULL才能终止参数列表。如果钩子不存在或不能执行，则返回值为零。如果它是可执行的，钩子将被执行，并返回钩子的退出状态。在执行时，将设置.stdout_to_stderr和.no_stdin。
 */
LAST_ARG_MUST_BE_NULL
int run_hook_le(const char *const *env, const char *name, ...);
int run_hook_ve(const char *const *env, const char *name, va_list args);

#define RUN_COMMAND_NO_STDIN 1
#define RUN_GIT_CMD 2 /*If this is to be git sub-command */
#define RUN_COMMAND_STDOUT_TO_STDERR 4
#define RUN_SILENT_EXEC_FAILURE 8
#define RUN_USING_SHELL 16
#define RUN_CLEAN_ON_EXIT 32

/**
 * Convenience functions that encapsulate a sequence of
 * start_command() followed by finish_command(). The argument argv
 * specifies the program and its arguments. The argument opt is zero
 * or more of the flags `RUN_COMMAND_NO_STDIN`, `RUN_GIT_CMD`,
 * `RUN_COMMAND_STDOUT_TO_STDERR`, or `RUN_SILENT_EXEC_FAILURE`
 * that correspond to the members .no_stdin, .git_cmd,
 * .stdout_to_stderr, .silent_exec_failure of `struct child_process`.
 * The argument dir corresponds the member .dir. The argument env
 * corresponds to the member .env.
 * 封装start_command()序列和finish_command()的方便函数。参数argv指定程序及其参数。参数opt是零个或多个标记'
 * RUN_COMMAND_NO_STDIN '、' RUN_GIT_CMD '、' RUN_COMMAND_STDOUT_TO_STDERR '或'
 * RUN_SILENT_EXEC_FAILURE '，这些标记对应于' struct child_process
 * '的成员。参数dir对应于成员.dir。参数env对应于成员.env。
 */
int run_command_v_opt(const char **argv, int opt);
int run_command_v_opt_tr2(const char **argv, int opt, const char *tr2_class);
/*
 * env (the environment) is to be formatted like environ: "VAR=VALUE".
 * To unset an environment variable use just "VAR".
 */
int run_command_v_opt_cd_env(const char **argv, int opt, const char *dir,
			     const char *const *env);
int run_command_v_opt_cd_env_tr2(const char **argv, int opt, const char *dir,
				 const char *const *env, const char *tr2_class);

/**
 * Execute the given command, sending "in" to its stdin, and capturing its
 * stdout and stderr in the "out" and "err" strbufs. Any of the three may
 * be NULL to skip processing.
 * 执行给定的命令，向其stdin发送“in”，并在“out”和“err”strbufs中捕获其stdout和stderr。这三个中的任何一个都可以为空以跳过处理。
 *
 * Returns -1 if starting the command fails or reading fails, and otherwise
 * returns the exit code of the command. Any output collected in the
 * buffers is kept even if the command returns a non-zero exit. The hint fields
 * gives starting sizes for the strbuf allocations.
 * 如果启动命令失败或读取失败，则返回-1，否则返回命令的退出码。即使该命令返回一个非零出口，缓冲区中收集的任何输出也将被保留。提示字段给出了strbuf分配的初始大小。
 *
 * The fields of "cmd" should be set up as they would for a normal run_command
 * invocation. But note that there is no need to set the in, out, or err
 * fields; pipe_command handles that automatically.
 * “cmd”字段的设置应该与正常的run_command调用一样。但是请注意，不需要设置in、out或err字段;pipe_command自动处理它。
 */
int pipe_command(struct child_process *cmd, const char *in, size_t in_len,
		 struct strbuf *out, size_t out_hint, struct strbuf *err,
		 size_t err_hint);

/**
 * Convenience wrapper around pipe_command for the common case
 * of capturing only stdout.
 * pipe_command的方便包装，用于只捕获stdout的常见情况。
 */
static inline int capture_command(struct child_process *cmd, struct strbuf *out,
				  size_t hint)
{
	return pipe_command(cmd, NULL, 0, out, hint, NULL, 0);
}

/*
 * The purpose of the following functions is to feed a pipe by running
 * a function asynchronously and providing output that the caller reads.
 * 以下函数的目的是通过异步运行函数并提供调用者读取的输出来提供管道。
 *
 * It is expected that no synchronization and mutual exclusion between
 * the caller and the feed function is necessary so that the function
 * can run in a thread without interfering with the caller.
 * 预期调用者和feed函数之间不需要同步和互斥，这样函数就可以在线程中运行而不会干扰调用者。
 *
 * The caller:
 *
 * 1. allocates and clears (memset(&asy, 0, sizeof(asy));) a
 *    struct async variable;
 * 2. initializes .proc and .data;
 * 3. calls start_async();
 * 4. processes communicates with proc through .in and .out;
 * 5. closes .in and .out;
 * 6. calls finish_async().
 *
 * There are serious restrictions on what the asynchronous function can do
 * because this facility is implemented by a thread in the same address
 * space on most platforms (when pthreads is available), but by a pipe to
 * a forked process otherwise:
 *
 * - It cannot change the program's state (global variables, environment,
 *   etc.) in a way that the caller notices; in other words, .in and .out
 *   are the only communication channels to the caller.
 *
 * - It must not change the program's state that the caller of the
 *   facility also uses.
 *
 */
struct async {
	/**
	 * The function pointer in .proc has the following signature:
	 *
	 *	int proc(int in, int out, void *data);
	 *
	 * - in, out specifies a set of file descriptors to which the function
	 *  must read/write the data that it needs/produces.  The function
	 *  *must* close these descriptors before it returns.  A descriptor
	 *  may be -1 if the caller did not configure a descriptor for that
	 *  direction.
	 *
	 * - data is the value that the caller has specified in the .data member
	 *  of struct async.
	 *
	 * - The return value of the function is 0 on success and non-zero
	 *  on failure. If the function indicates failure, finish_async() will
	 *  report failure as well.
	 *
	 */
	int (*proc)(int in, int out, void *data);

	void *data;

	/**
	 * The members .in, .out are used to provide a set of fd's for
	 * communication between the caller and the callee as follows:
	 *
	 * - Specify 0 to have no file descriptor passed.  The callee will
	 *   receive -1 in the corresponding argument.
	 *
	 * - Specify < 0 to have a pipe allocated; start_async() replaces
	 *   with the pipe FD in the following way:
	 *
	 * 	.in: Returns the writable pipe end into which the caller
	 * 	writes; the readable end of the pipe becomes the function's
	 * 	in argument.
	 *
	 * 	.out: Returns the readable pipe end from which the caller
	 * 	reads; the writable end of the pipe becomes the function's
	 * 	out argument.
	 *
	 *   The caller of start_async() must close the returned FDs after it
	 *   has completed reading from/writing from them.
	 *
	 * - Specify a file descriptor > 0 to be used by the function:
	 *
	 * 	.in: The FD must be readable; it becomes the function's in.
	 * 	.out: The FD must be writable; it becomes the function's out.
	 *
	 *   The specified FD is closed by start_async(), even if it fails to
	 *   run the function.
	 */
	int in; /* caller writes here and closes it */
	int out; /* caller reads from here and closes it */
#ifdef NO_PTHREADS
	pid_t pid;
#else
	pthread_t tid;
	int proc_in;
	int proc_out;
#endif
	int isolate_sigpipe;
};

/**
 * Run a function asynchronously. Takes a pointer to a `struct
 * async` that specifies the details and returns a set of pipe FDs
 * for communication with the function. See below for details.
 * 异步运行一个函数。获取指向指定详细信息的‘async结构’的指针，并返回一组用于与函数通信的管道fd。详见下文。
 */
int start_async(struct async *async);

/**
 * Wait for the completion of an asynchronous function that was
 * started with start_async().
 * 等待start_async()启动的异步函数完成。
 */
int finish_async(struct async *async);

int in_async(void);
int async_with_fork(void);
void check_pipe(int err);

/**
 * This callback should initialize the child process and preload the
 * error channel if desired. The preloading of is useful if you want to
 * have a message printed directly before the output of the child process.
 * pp_cb is the callback cookie as passed to run_processes_parallel.
 * You can store a child process specific callback cookie in pp_task_cb.
 * 这个回调应该初始化子进程，并在需要时预加载错误通道。如果希望在子进程的输出之前直接打印一条消息，则可以使用预加载。pp_cb是传递给run_processes_parallel的回调cookie。您可以在pp_task_cb中存储一个子进程特定的回调cookie。
 *
 * Even after returning 0 to indicate that there are no more processes,
 * this function will be called again until there are no more running
 * child processes.
 * 即使在返回0以表示没有更多的进程之后，也会再次调用这个函数，直到没有更多正在运行的子进程为止。
 *
 * Return 1 if the next child is ready to run.
 * Return 0 if there are currently no more tasks to be processed.
 * To send a signal to other child processes for abortion,
 * return the negative signal number.
 * 如果下一个子元素准备运行，则返回1。如果当前没有更多的任务需要处理，则返回0。若要向其他子进程发送堕胎信号，请返回负信号号。
 */
typedef int (*get_next_task_fn)(struct child_process *cp, struct strbuf *out,
				void *pp_cb, void **pp_task_cb);

/**
 * This callback is called whenever there are problems starting
 * a new process.
 * 每当启动新进程出现问题时，都会调用此回调。
 *
 * You must not write to stdout or stderr in this function. Add your
 * message to the strbuf out instead, which will be printed without
 * messing up the output of the other parallel processes.
 * 在这个函数中，不能写入stdout或stderr。相反，将您的消息添加到strbuf中，它将被打印出来，而不会打乱其他并行进程的输出。
 *
 * pp_cb is the callback cookie as passed into run_processes_parallel,
 * pp_task_cb is the callback cookie as passed into get_next_task_fn.
 * pp_cb是传递给run_processes_parallel的回调cookie,
 * pp_task_cb是传递给get_next_task_fn的回调cookie。
 *
 * Return 0 to continue the parallel processing. To abort return non zero.
 * To send a signal to other child processes for abortion, return
 * the negative signal number.
 * 返回0以继续并行处理。中止返回非零。若要向其他子进程发送堕胎信号，请返回负信号号。
 */
typedef int (*start_failure_fn)(struct strbuf *out, void *pp_cb,
				void *pp_task_cb);

/**
 * This callback is called on every child process that finished processing.
 * 在完成处理的每个子进程上调用此回调。
 *
 * You must not write to stdout or stderr in this function. Add your
 * message to the strbuf out instead, which will be printed without
 * messing up the output of the other parallel processes.
 * 在这个函数中，不能写入stdout或stderr。相反，将您的消息添加到strbuf中，它将被打印出来，而不会打乱其他并行进程的输出。
 *
 * pp_cb is the callback cookie as passed into run_processes_parallel,
 * pp_task_cb is the callback cookie as passed into get_next_task_fn.
 * pp_cb是传递给run_processes_parallel的回调cookie,
 * pp_task_cb是传递给get_next_task_fn的回调cookie。
 *
 * Return 0 to continue the parallel processing.  To abort return non zero.
 * To send a signal to other child processes for abortion, return
 * the negative signal number.
 * 返回0以继续并行处理。中止返回非零。若要向其他子进程发送堕胎信号，请返回负信号号。
 */
typedef int (*task_finished_fn)(int result, struct strbuf *out, void *pp_cb,
				void *pp_task_cb);

/**
 * Runs up to n processes at the same time. Whenever a process can be
 * started, the callback get_next_task_fn is called to obtain the data
 * required to start another child process.
 * 同时运行最多n个进程。只要可以启动一个进程，就会调用get_next_task_fn回调函数来获取启动另一个子进程所需的数据。
 *
 * The children started via this function run in parallel. Their output
 * (both stdout and stderr) is routed to stderr in a manner that output
 * from different tasks does not interleave.
 * 子进程通过这个函数并行运行。它们的输出(stdout和stderr)以不同任务的输出不交错的方式路由到stderr。
 *
 * start_failure_fn and task_finished_fn can be NULL to omit any
 * special handling.
 * start_failure_fn和task_finished_fn可以为空，以忽略任何特殊处理。
 */
int run_processes_parallel(int n, get_next_task_fn, start_failure_fn,
			   task_finished_fn, void *pp_cb);
int run_processes_parallel_tr2(int n, get_next_task_fn, start_failure_fn,
			       task_finished_fn, void *pp_cb,
			       const char *tr2_category, const char *tr2_label);

#endif
