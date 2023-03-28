//
// Copyright (c) 2010-2018 Antmicro
//
//  This file is licensed under the MIT License.
//  Full license text is available in 'licenses/MIT.txt'.
//
using Antmicro.Renode.Utilities.Packets;

namespace Antmicro.Renode.Storage.SCSI.Commands
{
    public struct ReadCapcity10Result
    {
        [PacketField]
        public uint ReturnedLogicalBlockAddress;
        [PacketField]
        public uint BlockLengthInBytes;
    }
}