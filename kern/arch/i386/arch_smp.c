#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <aim/smp.h>
#include <aim/vmm.h>
#include <aim/panic.h>
#include <arch/i386/asm.h>
#include <arch/i386/arch-mmu.h>
#include <aim/mmu.h>
#include <aim/early_kmmap.h>
#include <aim/console.h>
#include <aim/trap.h>
/* based on the impletation from xv6 */


#define CMOS_PORT 0x70
// Local APIC registers, divided by 4 for use as uint[] indices.
#define ID      (0x0020/4)   // ID
#define VER     (0x0030/4)   // Version
#define TPR     (0x0080/4)   // Task Priority
#define EOI     (0x00B0/4)   // EOI
#define SVR     (0x00F0/4)   // Spurious Interrupt Vector
  #define ENABLE     0x00000100   // Unit Enable
#define ESR     (0x0280/4)   // Error Status
#define ICRLO   (0x0300/4)   // Interrupt Command
  #define INIT       0x00000500   // INIT/RESET
  #define STARTUP    0x00000600   // Startup IPI
  #define DELIVS     0x00001000   // Delivery status
  #define ASSERT     0x00004000   // Assert interrupt (vs deassert)
  #define DEASSERT   0x00000000
  #define LEVEL      0x00008000   // Level triggered
  #define BCAST      0x00080000   // Send to all APICs, including self.
  #define BUSY       0x00001000
  #define FIXED      0x00000000
#define ICRHI   (0x0310/4)   // Interrupt Command [63:32]
#define TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
  #define X1         0x0000000B   // divide counts by 1
  #define PERIODIC   0x00020000   // Periodic
#define PCINT   (0x0340/4)   // Performance Counter LVT
#define LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
#define LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
#define ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
  #define MASKED     0x00010000   // Interrupt masked
#define TICR    (0x0380/4)   // Timer Initial Count
#define TCCR    (0x0390/4)   // Timer Current Count
#define TDCR    (0x03E0/4)   // Timer Divide Configuration

#define LAPIC_BASE (0xfee00000)
#define LAPIC_SIZE (0x1000)
#define SHIFT_LEFT ((uint32_t)(1<<24))

struct mp_floating_pointer_structure* mpsearch1(uint32_t a, int len)
{
  uint8_t *e, *p, *addr;

  addr = pa2kva(a);
  e = addr+len;
  for(p = addr; p < e; p += sizeof(mp_str))
    if(memcmp(p, "_MP_", 4) == 0)
      return (mp_str*)p;
  return 0;
}

struct mp_floating_pointer_structure* find_mptable(void)
{
    struct mp_floating_pointer_structure* mp;
    struct mp_configuration_table* conf; 
    uint8_t *bda;
    uint32_t p;
    bda = (uint8_t*) pa2kva(0x400);
    if((p = ((bda[0x0F]<<8)| bda[0x0E]) << 4)){
      if((mp = mpsearch1(p, 1024)))
        return mp;
    } else {
      p = ((bda[0x14]<<8)|bda[0x13])*1024;
      if((mp = mpsearch1(p-1024, 1024)))
        return mp;
    }
    return mpsearch1(0xF0000, 0x10000);
}

struct mp_configuration_table* find_mpconf(mp_str* mp)
{
    mpconf *conf;
    if((mp == 0 || mp->configuration_table == 0))
      return 0;
    conf = (mpconf*) pa2kva((uint32_t) mp->configuration_table);
    if(memcmp(conf, "PCMP", 4) != 0)
      return 0;
    if(conf->mp_specification_revision != 1 && conf->mp_specification_revision != 4)
      return 0;
    return conf;
}

static uint8_t global_cpus[MAX_CPU_NUM];
static uint32_t global_cpunms;

uint32_t find_smp(uint8_t** cpu_id,uint32_t* io_apicid)
{
    uint8_t *p, *e;
    int ismp=1;
    struct mp_floating_pointer_structure* mp;
    struct mp_configuration_table* conf; 
    struct entry_processor* proc;
    struct entry_io_apic* io_apic;
    mp=find_mptable();
    conf=find_mpconf(mp);
    kprintf("Lapic address:0x%x\n",conf->lapic_address);
    uint8_t* cpus=kmalloc(sizeof(uint8_t)*MAX_CPU_NUM,0);
    uint32_t ncpu=0;
    for(p=(uint8_t*)(conf+1), e=(uint8_t*)conf+conf->length; p<e; )
    {
        switch(*p){
        case MPPROC:
          proc = (struct entry_processor*)p;
          if(ncpu < MAX_CPU_NUM) {
            kprintf("apicid:0x%x\n",proc->local_apic_id);
            cpus[ncpu]= proc->local_apic_id;  // apicid may differ from ncpu
            global_cpus[ncpu]=proc->local_apic_id;
            ncpu++;
          }
          p += sizeof(struct entry_processor);
          continue;
        case MPIOAPIC:
          io_apic = (struct entry_io_apic*)p;
          *io_apicid = io_apic->id;
          p += sizeof(struct entry_io_apic);
          continue;
        case MPBUS:
        case MPIOINTR:
        case MPLINTR:
          p += 8;
          continue;
        default:
          ismp = 0;
          break;
        }
    }
    if(!ismp)
        panic("Didn't find a suitable machine");
    *cpu_id=cpus;
    global_cpunms=ncpu;
    return ncpu;
    /*
      if(uint8_t(mp->features)){
        // Bochs doesn't support IMCR, so this doesn't run on Bochs.
        // But it would on real hardware.
        outb(0x22, 0x70);   // Select IMCR
        outb(0x23, inb(0x23) | 1);  // Mask external interrupts.
      }
    */
}

static int* lapic=(int*)(LAPIC_BASE);

void lapicw(int index, int value)
{
  lapic[index] = value;
  lapic[ID];  // wait for write to finish, by reading
}

void microdelay(uint32_t time)
{
    kprintf("delay %d ms\n",time);
    for(volatile uint32_t i=0;i<time*20000;i+=2)
      if(i%2==1)
        break;
        //kprintf("%d s passing\n",(i+1)/10000);
    return;
}

void lapicstartap(uint8_t apicid, uint32_t addr)
{
  int i;
  uint16_t *wrv;

  outb(CMOS_PORT, 0xF);  
  outb(CMOS_PORT+1, 0x0A);
  wrv = (uint16_t*)pa2kva((0x40<<4 | 0x67));  // Warm reset vector
  wrv[0] = 0;
  wrv[1] = (uint16_t)(addr >> 4);

  lapicw(ICRHI, ((int)apicid<<24));
  lapicw(ICRLO, INIT | LEVEL | ASSERT);
  microdelay(200);
  lapicw(ICRLO, INIT | LEVEL);
  microdelay(100);    

  // Send startup IPI (twice!) to enter code.
  for(i = 0; i < 2; i++)
  {
    lapicw(ICRHI, ((int)apicid)<<24);
    lapicw(ICRLO, STARTUP | (addr>>12));
    microdelay(200);
  }
  kprintf("waking up cpu:0x%x\n",apicid);
}

void lapic_mapping(void)
{
  struct early_mapping entry = {
		.paddr	= LAPIC_BASE,
		.vaddr	= LAPIC_BASE,
		.size	= (size_t)LAPIC_SIZE,
		.type	= EARLY_MAPPING_KMMAP
	};

	if (early_mapping_add(&entry) < 0)
		panic("fail to mapping lapic");
}

uint8_t cpuid(void)
{
  return (uint8_t)(lapic[ID]>>24);
}

static volatile uint32_t cpu_status[MAX_CPU_NUM]={0,0,0,0,0,0,0,0,0,0,0,0};

void other_init(void)
{
  uint8_t cpu_id=cpuid();
  lapicinit();
  other_trap_init();
  kprintf("cpu No.0x%x awake.\n",cpu_id);
  cpu_status[cpu_id]=1;
  while(1);
}
extern char _otherstart[],_otherend[];

void lapicinit(void)
{
  if(!lapic)
    return;

  // Enable local APIC; set spurious interrupt vector.
  lapicw(SVR, ENABLE | (32 + IRQ_SPURIOUS));


  // Disable logical interrupt lines.
  lapicw(LINT0, MASKED);
  lapicw(LINT1, MASKED);
  lapicw(ERROR, MASKED);

  // Clear error status register (requires back-to-back writes).
  lapicw(ESR, 0);
  lapicw(ESR, 0);

  // Ack any outstanding interrupts.
  lapicw(EOI, 0);

  // Send an Init Level De-Assert to synchronise arbitration ID's.
  lapicw(ICRHI, 0);
  lapicw(ICRLO, BCAST | INIT | LEVEL);
  while(lapic[ICRLO] & DELIVS)
    ;

  // Enable interrupts on the APIC (but not on the processor).
  lapicw(TPR, 0);
}

void smp_startup(void)
{
  uint8_t cpu_id=cpuid();
  uint8_t* cpuids=NULL;
  uint32_t io_apicid=NULL;
  uint32_t cpu_nums=find_smp(&cpuids,&io_apicid);
  ioapicinit(io_apicid);
  lapicinit();
  uint32_t code_addr=0x8000;
  for(char* src=_otherstart,*dst=code_addr;src<=_otherend;src++,dst++)
    *(dst)=*(src);
  *(uint32_t*)(code_addr-4)=other_init;
  uint32_t stack=(uint32_t)kmalloc(PAGE_SIZE / 2,0);
  *(uint32_t*)(code_addr-12)=stack;
  *(uint32_t*)(code_addr-8)=(uint32_t)get_pgindex();
  kprintf("master cpu id:0x%x\n",cpu_id);
  kprintf("cpu nums:%d\n",cpu_nums);
  for(uint32_t i=0;i<cpu_nums;i++)
  {
    kprintf("prepare cpu:0x%x\n",global_cpus[i]);
    if(global_cpus[i]==cpu_id)
      continue;
    lapicstartap(global_cpus[i],code_addr);
    kprintf("waiting for cpu:0x%x\n",global_cpus[i]);
    while(cpu_status[global_cpus[i]]==0);
  }
  microdelay(2000);
  kprintf("master leave\n");
  return;
}

void panic_other_cpus(void)
{
  uint8_t cpu_id=cpuid();
  for(int i=0;i<global_cpunms;i++)
  {
    if(global_cpus[i]==cpu_id)
      continue;
    lapicw(ICRHI, global_cpus[i]<<24);
    lapicw(ICRLO, FIXED | ASSERT | T_IPI);
    microdelay(200);
  }
  set_mp();
  return;
}