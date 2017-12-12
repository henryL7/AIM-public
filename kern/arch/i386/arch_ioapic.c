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

#define IOAPIC  0xFEC00000   // Default physical address of IO APIC

#define REG_ID     0x00  // Register index: ID
#define REG_VER    0x01  // Register index: version
#define REG_TABLE  0x10  // Redirection table base

// The redirection table starts at REG_TABLE and uses
// two registers to configure each interrupt.
// The first (low) register in a pair contains configuration bits.
// The second (high) register contains a bitmask telling which
// CPUs can serve that interrupt.
#define INT_DISABLED   0x00010000  // Interrupt disabled
#define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
#define INT_ACTIVELOW  0x00002000  // Active low (vs high)
#define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)

volatile struct ioapic *ioapic;

// IO APIC MMIO structure: write reg, then read or write data.
struct ioapic {
  uint32_t reg;
  uint32_t pad[3];
  uint32_t data;
};__attribute__((packed));

static uint32_t ioapicread(int reg)
{
  ioapic->reg = reg;
  return ioapic->data;
}

static void ioapicwrite(int reg, uint32_t data)
{
  ioapic->reg = reg;
  ioapic->data = data;
}

void ioapicinit(int ioapicid)
{
  int i, id, maxintr;

  ioapic = (volatile struct ioapic*)IOAPIC;
  maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;
  id = ioapicread(REG_ID) >> 24;
  if(id != ioapicid)
    kprintf("ioapicinit: id isn't equal to ioapicid; not a MP\n");

  // Mark all interrupts edge-triggered, active high, disabled,
  // and not routed to any CPUs.
  for(i = 0; i <= maxintr; i++){
    ioapicwrite(REG_TABLE+2*i, INT_DISABLED | (32 + i));
    ioapicwrite(REG_TABLE+2*i+1, 0);
  }
}

void ioapicenable(int irq, int cpunum)
{
  // Mark interrupt edge-triggered, active high,
  // enabled, and routed to the given cpunum,
  // which happens to be that cpu's APIC ID.
  ioapicwrite(REG_TABLE+2*irq, 32 + irq);
  ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);
}