/*
 * Copyright (c) 2020, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/IntrusiveList.h>
#include <Kernel/Devices/BlockDevice.h>
#include <Kernel/Interrupts/IRQHandler.h>
#include <Kernel/Locking/Mutex.h>
#include <Kernel/Storage/DiskPartition.h>
#include <Kernel/Storage/StorageController.h>

namespace Kernel {

class RamdiskDevice;
class StorageDevice : public BlockDevice {
    friend class StorageManagement;
    friend class DeviceManagement;

public:
    // Note: this attribute describes the internal command set of a Storage device.
    // For example, an ordinary harddrive utilizes the ATA command set, while
    // an ATAPI device (e.g. Optical drive) that is connected to the ATA bus,
    // is actually using SCSI commands (packets) encapsulated inside an ATA command.
    // The IDE controller code being aware of the possibility of ATAPI devices attached
    // to the ATA bus, will check whether the Command set is ATA or SCSI and will act
    // accordingly.
    // Note: For now, there's simply no distinction between the interface type and the commandset.
    // As mentioned above, ATAPI devices use the ATA interface with actual SCSI packets so
    // the commandset is SCSI while the interface type is ATA. We simply don't support SCSI over ATA (ATAPI)
    // and ATAPI is the exception to no-distinction rule. If we ever put SCSI support in the kernel,
    // we can create another enum class to put the distinction.
    enum class CommandSet {
        PlainMemory,
        SCSI,
        ATA,
        NVMe,
    };

    // Note: The most reliable way to address this device from userspace interfaces,
    // such as SysFS, is to have one way to enumerate everything in the eyes of userspace.
    // Therefore, SCSI LUN (logical unit number) addressing seem to be the most generic way to do this.
    // For example, on a legacy ATA instance, one might connect an harddrive to the second IDE controller,
    // to the Primary channel as a slave device, which translates to LUN 1:0:1.
    // On NVMe, for example, connecting a second PCIe NVMe storage device as a sole NVMe namespace translates
    // to LUN 1:0:0.
    struct LUNAddress {
        u32 controller_id;
        u32 target_id;
        u32 disk_id;
    };

public:
    virtual u64 max_addressable_block() const { return m_max_addressable_block; }

    // ^BlockDevice
    virtual ErrorOr<size_t> read(OpenFileDescription&, u64, UserOrKernelBuffer&, size_t) override;
    virtual bool can_read(OpenFileDescription const&, u64) const override;
    virtual ErrorOr<size_t> write(OpenFileDescription&, u64, UserOrKernelBuffer const&, size_t) override;
    virtual bool can_write(OpenFileDescription const&, u64) const override;
    virtual void prepare_for_unplug() { m_partitions.clear(); }

    NonnullLockRefPtrVector<DiskPartition> const& partitions() const { return m_partitions; }

    void add_partition(NonnullLockRefPtr<DiskPartition> disk_partition) { MUST(m_partitions.try_append(disk_partition)); }

    LUNAddress const& logical_unit_number_address() const { return m_logical_unit_number_address; }

    u32 parent_controller_hardware_relative_id() const { return m_hardware_relative_controller_id; }

    virtual CommandSet command_set() const = 0;

    StringView command_set_to_string_view() const;

    // ^File
    virtual ErrorOr<void> ioctl(OpenFileDescription&, unsigned request, Userspace<void*> arg) final;

protected:
    StorageDevice(LUNAddress, u32 hardware_relative_controller_id, size_t sector_size, u64);

    // Note: We want to be able to put distinction between Storage devices and Ramdisk-based devices.
    // We do this because it will make selecting ramdisk devices much more easier in boot time in the kernel commandline.
    StorageDevice(Badge<RamdiskDevice>, LUNAddress, u32 hardware_relative_controller_id, MajorNumber, MinorNumber, size_t sector_size, u64);

    // ^DiskDevice
    virtual StringView class_name() const override;

private:
    virtual void after_inserting() override;
    virtual void will_be_destroyed() override;

    mutable IntrusiveListNode<StorageDevice, LockRefPtr<StorageDevice>> m_list_node;
    NonnullLockRefPtrVector<DiskPartition> m_partitions;

    LUNAddress const m_logical_unit_number_address;

    // Note: This data member should be used with LUNAddress target_id and disk_id.
    // LUNs are agnostic system-wide addresses, so they are assigned without caring about the specific hardware interfaces.
    // This class member on the other side, is meant to be assigned *per hardware type*,
    // which means in constrast to the LUNAddress controller_id struct member, we take the index of the hardware
    // controller among its fellow controllers of the same hardware type in the system.
    u32 const m_hardware_relative_controller_id { 0 };

    u64 m_max_addressable_block { 0 };
    size_t m_blocks_per_page { 0 };
};

}
