#!/bin/bash
set -euo pipefail

ROOT="${ROOT:-vitteos}"
SCALE="${SCALE:-2}"     # SCALE=2 (défaut), SCALE=4 (énorme), SCALE=8 (extrême)

# -----------------------------
# Counts (× SCALE)
# -----------------------------
N_FS=$((220 * SCALE))
N_VFS=$((320 * SCALE))
N_EXT2=$((180 * SCALE))
N_EXT4=$((360 * SCALE))
N_TMPFS=$((140 * SCALE))
N_PROCFS=$((160 * SCALE))
N_DEVFS=$((160 * SCALE))

N_NET=$((260 * SCALE))
N_IP4=$((220 * SCALE))
N_IP6=$((220 * SCALE))
N_TCP=$((520 * SCALE))
N_UDP=$((260 * SCALE))
N_NETDRV=$((220 * SCALE))
N_NETFILTER=$((220 * SCALE))

N_DRV=$((520 * SCALE))
N_PCI=$((280 * SCALE))
N_USB=$((360 * SCALE))
N_BLK=$((360 * SCALE))
N_GPU=$((260 * SCALE))
N_INPUT=$((220 * SCALE))
N_CHAR=$((260 * SCALE))
N_ACPI=$((220 * SCALE))
N_VIRTIO=$((220 * SCALE))
N_POWER=$((220 * SCALE))

N_MM=$((520 * SCALE))
N_PAGING=$((360 * SCALE))
N_BUDDY=$((260 * SCALE))
N_SLAB=$((260 * SCALE))
N_VMA=$((220 * SCALE))
N_MMAP=$((220 * SCALE))
N_NUMA=$((180 * SCALE))

N_ARCH_X86=$((620 * SCALE))
N_ARCH_A64=$((520 * SCALE))
N_ARCH_RV=$((420 * SCALE))
N_BOOT=$((240 * SCALE))
N_SYSCALL=$((320 * SCALE))
N_SECURITY=$((320 * SCALE))
N_DEBUG=$((320 * SCALE))
N_TEST=$((420 * SCALE))

# Raspberry Pi / SoC
N_RPI=$((900 * SCALE))
N_BCM2835=$((420 * SCALE))
N_BCM2711=$((520 * SCALE))
N_VC4=$((420 * SCALE))
N_BRCMFMAC=$((360 * SCALE))
N_DTO=$((520 * SCALE))

echo "[i] ROOT=$ROOT  SCALE=$SCALE"
echo "[i] Objectif: Linux-like tree + Raspberry Pi + fichiers .vit vides (massifs)"

echo "[+] Création arborescence (base)..."
mkdir -p "$ROOT"/{arch/{x86_64/{boot,gdt,idt,syscall,paging,apic,smp,cpu},i386,aarch64,riscv64,common},boot/{uefi,bios,multiboot,loader,linker},core/{entry,panic,klog,config,buildinfo,version},mm/{paging,frame_allocator,slab,buddy,vma,mmap,heap,kalloc,numa},sched/{scheduler,task,thread,context_switch,load_balance,realtime},ipc/{pipe,message_queue,shared_memory,futex,signal},fs/{vfs,inode,dentry,superblock,ext2,ext4,tmpfs,procfs,devfs,mount},net/{socket,tcp,udp,ip,icmp,arp,driver,netfilter},drivers/{block,char,gpu,input,usb,pci,acpi,virtio,power},security/{capability,selinux_like,sandbox,audit,namespace},syscall/{table,dispatch,abi,compat,tracing},module/{loader,reloc,symbol,dependency,hotplug},power/{suspend,resume,cpufreq,thermal},virt/{kvm_like,hypervisor,vmexit,guest},debug/{gdb_stub,ktrace,profiler,ftrace,crashdump},test/{kernel_tests,mm_tests,fs_tests,net_tests,sched_tests},include/{api,uapi,internal},build/{configs,toolchain,scripts}}

echo "[+] Ajout sous-dossiers deep (Linux-like)..."
mkdir -p \
  "$ROOT"/arch/x86_64/{msr,tss,exceptions,io,lapic,ioapic,tsc,cpuid,features,pat,mtrr,vmx,svm,pmu,perf} \
  "$ROOT"/arch/x86_64/boot/{early,stage1,stage2,acpi,cmdline,memmap,efi_stub} \
  "$ROOT"/arch/x86_64/paging/{pml4,pdpt,pd,pt,tlb} \
  "$ROOT"/arch/x86_64/syscall/{entry,msr,gate,abi,vsyscall,vdso_like} \
  "$ROOT"/arch/x86_64/smp/{ipi,topology,bringup,hotplug} \
  "$ROOT"/arch/aarch64/{boot,mmu,exceptions,irq,timer,cpu,psci,smccc,cache,tlb,sysreg,gic,el,pmu} \
  "$ROOT"/arch/aarch64/boot/{early,dtb,uefi,firmware} \
  "$ROOT"/arch/riscv64/{boot,mmu,irq,timer,cpu,sbi,csr,tlb,plic,clint} \
  "$ROOT"/arch/common/{dt,mmio,fw,platform,irq,clock,reset} \
  "$ROOT"/core/{assert,spinlock,atomic,init,shutdown,rand,abi,trace,printk,workqueue,rcu_like} \
  "$ROOT"/core/klog/{sink,ring,fmt} \
  "$ROOT"/mm/{kmem,vm,pm,guard,cache,hugepages,kaslr,swap,oom,vmfault,kasan_like} \
  "$ROOT"/mm/paging/{walker,mapper,unmap,protect} \
  "$ROOT"/mm/buddy/{zones,compact,stats} \
  "$ROOT"/mm/slab/{magazine,quarantine,stats} \
  "$ROOT"/mm/vma/{tree,merge,split,protect} \
  "$ROOT"/mm/mmap/{file,anon,shared,private} \
  "$ROOT"/mm/numa/{topology,policy,balancer} \
  "$ROOT"/sched/{waitq,timer,rt,prio,cpu_affinity,cgroup_like,preempt,loadavg,deadline} \
  "$ROOT"/ipc/{chan,mailbox,rpc,bus,shared_ring,eventfd_like} \
  "$ROOT"/fs/{path,perm,cache,buffer,block,quota,journal,notify,locks,xattr,io} \
  "$ROOT"/fs/vfs/{namei,lookup,open,read,write,ioctl,readdir,stat,link,unlink,rename,chmod,chown,mmap,pipefs,fdtable} \
  "$ROOT"/net/{route,iface,ether,util,phy,neigh,skbuff_like} \
  "$ROOT"/net/ip/{v4,v6,fragment,reassembly,checksum} \
  "$ROOT"/net/tcp/{state,rx,tx,retrans,timer,congestion,options,sack,keepalive,fastopen_like} \
  "$ROOT"/net/udp/{rx,tx} \
  "$ROOT"/net/driver/{e1000,rtl8139,virtio_net} \
  "$ROOT"/net/netfilter/{core,nat,conntrack,match,target} \
  "$ROOT"/drivers/pci/{config,enumerate,msi,msix,bar,irq,quirks} \
  "$ROOT"/drivers/usb/{core,hub,ehci,uhci,xhci,dwc2,gadget} \
  "$ROOT"/drivers/block/{ahci,ata,nvme,virtio_blk,ramdisk,scsi,partition} \
  "$ROOT"/drivers/input/{kbd,mouse,evdev,serio,hid_like} \
  "$ROOT"/drivers/gpu/{fb,vesa,drm_like} \
  "$ROOT"/drivers/char/{tty,serial,console,random,mem} \
  "$ROOT"/drivers/acpi/{tables,aml_like,ec,pci} \
  "$ROOT"/drivers/virtio/{transport,queue,pci,mmio} \
  "$ROOT"/drivers/power/{battery,charger,regulator,clock,reset,pm} \
  "$ROOT"/security/{cred,uidgid,acl,mac,seccomp_like,keys,lsm_like,capability} \
  "$ROOT"/syscall/abi/{linux_like,posix_like} \
  "$ROOT"/syscall/tracing/{kprobe_like,uprobes_like,tracepoints} \
  "$ROOT"/module/loader/{elf,reloc,plt,gottable,symbols} \
  "$ROOT"/debug/{symbols,log,serial,console,hexdump} \
  "$ROOT"/debug/crashdump/{elfcore,backtrace,stackwalk} \
  "$ROOT"/test/{harness,fuzz,bench} \
  "$ROOT"/include/uapi/{types,errno,syscalls} \
  "$ROOT"/include/internal/{mm,fs,net,sched,arch,drivers,security,debug}

echo "[+] Ajout Raspberry Pi (SoC/boards/drivers)..."
mkdir -p \
  "$ROOT"/platform/raspberrypi/{common,firmware,dt,boot,mailbox,property_tags,vc,clock,reset,power,watchdog} \
  "$ROOT"/platform/raspberrypi/boards/{rpi0,rpi1,rpi2,rpi3,rpi4,rpi5,cm3,cm4,cm5} \
  "$ROOT"/platform/raspberrypi/soc/{bcm2835,bcm2836,bcm2837,bcm2711,bcm2712} \
  "$ROOT"/platform/raspberrypi/irq/{armctrl,gic,local_intc} \
  "$ROOT"/platform/raspberrypi/mmio/{peripherals,mailbox,dma,clock,reset,gpio,uart,spi,i2c,pwm,pcm,timer} \
  "$ROOT"/platform/raspberrypi/dt/{overlays,bindings,compiler} \
  "$ROOT"/platform/raspberrypi/boot/{start4,fixup,config_txt,cmdline_txt} \
  "$ROOT"/drivers/soc/bcm/{dma,mailbox,clk,reset,pm,otp,thermal,irq} \
  "$ROOT"/drivers/gpu/vc4/{kms,hdmi,hvs,gem,irq,firmware,v3d} \
  "$ROOT"/drivers/char/serial/{pl011,mini_uart} \
  "$ROOT"/drivers/mmc/{sdhost,emmc,sdhci} \
  "$ROOT"/drivers/net/phy/{mdio,phycore} \
  "$ROOT"/drivers/net/wifi/{brcmfmac,brcmutil} \
  "$ROOT"/drivers/usb/{dwc2,xhci} \
  "$ROOT"/drivers/i2c/{bcm2835_i2c} \
  "$ROOT"/drivers/spi/{bcm2835_spi} \
  "$ROOT"/drivers/gpio/{bcm2835_gpio} \
  "$ROOT"/drivers/clk/{bcm2835_clk} \
  "$ROOT"/drivers/reset/{bcm2835_reset} \
  "$ROOT"/drivers/thermal/{bcm2835_thermal} \
  "$ROOT"/drivers/pwm/{bcm2835_pwm} \
  "$ROOT"/drivers/sound/{bcm2835_audio} \
  "$ROOT"/drivers/video/{bcm2835_camera} \
  "$ROOT"/arch/aarch64/platform/{raspberrypi} \
  "$ROOT"/arch/aarch64/platform/raspberrypi/{dt,firmware,mmio,bringup}

space_from_path() {
  local p="$1"
  p="${p#$ROOT/}"
  p="${p%.vit}"
  echo "$p"
}

TEMPLATE='<<< auto generated >>>

share all
'

write_empty_vit() {
  local file="$1"
  local sp
  sp="$(space_from_path "$file")"
  mkdir -p "$(dirname "$file")"
  printf "space %s\n\n%s" "$sp" "$TEMPLATE" > "$file"
}

echo "[+] mod.vit partout..."
while IFS= read -r d; do
  write_empty_vit "$d/mod.vit"
done < <(find "$ROOT" -type d)

echo "[+] fichiers standards par dossier..."
STD_FILES=(api types consts errors init impl tests cfg doc)
while IFS= read -r d; do
  for f in "${STD_FILES[@]}"; do
    write_empty_vit "$d/$f.vit"
  done
done < <(find "$ROOT" -type d)

gen_bulk() {
  local dir="$1"
  local prefix="$2"
  local n="$3"
  mkdir -p "$dir"
  for i in $(seq -w 1 "$n"); do
    write_empty_vit "$dir/${prefix}_${i}.vit"
  done
}

echo "[+] Explosion fichiers (Linux-like)..."
gen_bulk "$ROOT/fs"        "fs"     "$N_FS"
gen_bulk "$ROOT/fs/vfs"    "vfs"    "$N_VFS"
gen_bulk "$ROOT/fs/ext2"   "ext2"   "$N_EXT2"
gen_bulk "$ROOT/fs/ext4"   "ext4"   "$N_EXT4"
gen_bulk "$ROOT/fs/tmpfs"  "tmpfs"  "$N_TMPFS"
gen_bulk "$ROOT/fs/procfs" "procfs" "$N_PROCFS"
gen_bulk "$ROOT/fs/devfs"  "devfs"  "$N_DEVFS"

gen_bulk "$ROOT/net"            "net"   "$N_NET"
gen_bulk "$ROOT/net/ip/v4"      "ipv4"  "$N_IP4"
gen_bulk "$ROOT/net/ip/v6"      "ipv6"  "$N_IP6"
gen_bulk "$ROOT/net/tcp"        "tcp"   "$N_TCP"
gen_bulk "$ROOT/net/udp"        "udp"   "$N_UDP"
gen_bulk "$ROOT/net/driver"     "ndrv"  "$N_NETDRV"
gen_bulk "$ROOT/net/netfilter"  "nflt"  "$N_NETFILTER"

gen_bulk "$ROOT/drivers"         "drv"   "$N_DRV"
gen_bulk "$ROOT/drivers/pci"     "pci"   "$N_PCI"
gen_bulk "$ROOT/drivers/usb"     "usb"   "$N_USB"
gen_bulk "$ROOT/drivers/block"   "blk"   "$N_BLK"
gen_bulk "$ROOT/drivers/gpu"     "gpu"   "$N_GPU"
gen_bulk "$ROOT/drivers/input"   "in"    "$N_INPUT"
gen_bulk "$ROOT/drivers/char"    "ch"    "$N_CHAR"
gen_bulk "$ROOT/drivers/acpi"    "acpi"  "$N_ACPI"
gen_bulk "$ROOT/drivers/virtio"  "vio"   "$N_VIRTIO"
gen_bulk "$ROOT/drivers/power"   "pwr"   "$N_POWER"

gen_bulk "$ROOT/mm"         "mm"     "$N_MM"
gen_bulk "$ROOT/mm/paging"  "paging" "$N_PAGING"
gen_bulk "$ROOT/mm/buddy"   "buddy"  "$N_BUDDY"
gen_bulk "$ROOT/mm/slab"    "slab"   "$N_SLAB"
gen_bulk "$ROOT/mm/vma"     "vma"    "$N_VMA"
gen_bulk "$ROOT/mm/mmap"    "mmap"   "$N_MMAP"
gen_bulk "$ROOT/mm/numa"    "numa"   "$N_NUMA"

gen_bulk "$ROOT/arch/x86_64"  "x86"   "$N_ARCH_X86"
gen_bulk "$ROOT/arch/aarch64" "a64"   "$N_ARCH_A64"
gen_bulk "$ROOT/arch/riscv64" "rv"    "$N_ARCH_RV"
gen_bulk "$ROOT/arch/x86_64/boot" "boot" "$N_BOOT"

gen_bulk "$ROOT/syscall"   "sys"  "$N_SYSCALL"
gen_bulk "$ROOT/security"  "sec"  "$N_SECURITY"
gen_bulk "$ROOT/debug"     "dbg"  "$N_DEBUG"
gen_bulk "$ROOT/test"      "t"    "$N_TEST"

echo "[+] Explosion Raspberry Pi..."
gen_bulk "$ROOT/platform/raspberrypi"                     "rpi"      "$N_RPI"
gen_bulk "$ROOT/platform/raspberrypi/soc/bcm2835"         "bcm2835"  "$N_BCM2835"
gen_bulk "$ROOT/platform/raspberrypi/soc/bcm2711"         "bcm2711"  "$N_BCM2711"
gen_bulk "$ROOT/drivers/gpu/vc4"                          "vc4"      "$N_VC4"
gen_bulk "$ROOT/drivers/net/wifi/brcmfmac"                "brcmfmac" "$N_BRCMFMAC"
gen_bulk "$ROOT/platform/raspberrypi/dt/overlays"         "dto"      "$N_DTO"

echo "[+] entry.vit..."
cat << 'EOV' > "$ROOT/entry.vit"
space kernel/entry

<<< Kernel entry point >>>

pull core/entry

entry kernel at kernel/entry
  give core.entry.start()
.end
EOV

echo "[+] core/entry/mod.vit..."
cat << 'EOV' > "$ROOT/core/entry/mod.vit"
space core/entry

<<< Core entry >>>

share all

proc start() gives I32
  emit "VitteOS Kernel Booting..."
  give 0
.end
EOV

echo "[+] linker.ld..."
cat << 'EOV' > "$ROOT/build/linker.ld"
ENTRY(_start)

SECTIONS
{
  . = 1M;
  .text : { *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss : { *(.bss*) }
}
EOV

echo "[+] debug.conf..."
cat << 'EOV' > "$ROOT/build/configs/debug.conf"
KERNEL_NAME=VitteOS
ARCH=x86_64
DEBUG=true
EOV

echo "[+] README..."
cat << 'EOV' > "$ROOT/README.md"
# VitteOS Kernel MAX++++ (Linux-like, multi-arch, Raspberry Pi)
Généré automatiquement.
EOV

echo "[+] Stats..."
echo "Dossiers: $(find "$ROOT" -type d | wc -l)"
echo "Fichiers .vit: $(find "$ROOT" -type f -name '*.vit' | wc -l)"
echo "[✓] Génération terminée."
