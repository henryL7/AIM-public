#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <aim/smp.h>
#include <aim/vmm.h>
#include <aim/panic.h>
#include <arch/i386/asm.h>
#include <arch/i386/arch-mmu.h>


/* based on the impletation from xv6 */

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

uint32_t find_smp(uint32_t** cpu_id,uint32_t* io_apicid)
{
    uint8_t *p, *e;
    int ismp=0;
    struct mp_floating_pointer_structure* mp;
    struct mp_configuration_table* conf; 
    struct entry_processor* proc;
    struct entry_io_apic* io_apic;
    mp=find_mptable();
    conf=find_mpconf(mp);
    uint32_t* cpus=kmalloc(sizeof(uint32_t)*MAX_CPU_NUM,0);
    uint32_t ncpu=0;
    for(p=(uint8_t*)(conf+1), e=(uint8_t*)conf+conf->length; p<e; )
    {
        switch(*p){
        case MPPROC:
          proc = (struct entry_processor*)p;
          if(ncpu < MAX_CPU_NUM) {
            cpus[ncpu]= proc->local_apic_id;  // apicid may differ from ncpu
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