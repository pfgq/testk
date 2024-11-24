#include "process.h"
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/mm.h>
#include <linux/version.h>

#define ARC_PATH_MAX 256

extern struct mm_struct *get_task_mm(struct task_struct *task);

uintptr_t get_module_base(pid_t pid, char *name)
{
    struct pid *pid_struct;
    struct task_struct *task;
    struct mm_struct *mm;
    struct vm_area_struct *vma;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
    struct vma_iterator vmi;
#endif

    // 获取进程的 pid_struct
    pid_struct = find_get_pid(pid);
    if (!pid_struct)
    {
        return 0;  // 返回 0，表示找不到进程
    }

    // 获取任务结构体 task
    task = get_pid_task(pid_struct, PIDTYPE_PID);
    if (!task)
    {
        return 0;  // 返回 0，表示获取 task 失败
    }

    // 获取任务的 mm_struct
    mm = get_task_mm(task);
    if (!mm)
    {
        return 0;  // 返回 0，表示获取 mm_struct 失败
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 61))
    // 适用于 5.4.61 及以上版本
    mmput(mm);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
    // 在 4.19 版本上使用 exit_mmap
    down_write(&mm->mmap_sem); // 锁定 mmap
    exit_mmap(mm);
    up_write(&mm->mmap_sem);   // 解锁 mmap
#endif

    // 遍历进程的虚拟内存区域
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
    vma_iter_init(&vmi, mm, 0);
    for_each_vma(vmi, vma)
#else
    for (vma = mm->mmap; vma; vma = vma->vm_next)
#endif
    {
        char buf[ARC_PATH_MAX];
        char *path_nm = "";

        // 检查 VMA 是否映射文件
        if (vma->vm_file)
        {
            // 获取文件路径
            path_nm = file_path(vma->vm_file, buf, ARC_PATH_MAX - 1);
            // 如果文件名与目标模块名匹配，返回该 VMA 的起始地址
            if (!strcmp(kbasename(path_nm), name))
            {
                return vma->vm_start;
            }
        }
    }

    // 如果没有找到匹配的模块，返回 0
    return 0;
}
