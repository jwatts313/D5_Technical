//
// Copyright (c) 2010-2023 Antmicro
//
// This file is licensed under the MIT License.
// Full license text is available in 'licenses/MIT.txt'.
//
using System;
using System.Collections.Generic;

namespace Antmicro.Renode.Utilities.RESD
{
    public abstract class RESDSample
    {
        public virtual bool TryReadMetadata(SafeBinaryReader reader)
        {
            Metadata = MetadataBlock.ReadFromStream(reader);
            return true;
        }

        public virtual bool Skip(SafeBinaryReader reader, int count)
        {
            if(!Width.HasValue)
            {
                throw new RESDException($"This sample type ({this.GetType().Name}) doesn't allow for skipping data.");
            }

            if(reader.BaseStream.Position + count * Width.Value > reader.Length)
            {
                return false;
            }

            reader.SkipBytes(count * Width.Value);
            return true;
        }

        public abstract int? Width { get; }
        public abstract bool TryReadFromStream(SafeBinaryReader reader);

        public IDictionary<string, MetadataValue> Metadata { get; private set; }
    }

    [SampleType(SampleType.Temperature)]
    public class TemperatureSample : RESDSample
    {
        public override int? Width => 4;

        public int Temperature { get; private set; }

        public override bool TryReadFromStream(SafeBinaryReader reader)
        {
            Temperature = reader.ReadInt32();

            return true;
        }
    }

    [SampleType(SampleType.Acceleration)]
    public class AccelerationSample : RESDSample
    {
        public override int? Width => 4 * 3;

        public int AccelerationX { get; private set; }
        public int AccelerationY { get; private set; }
        public int AccelerationZ { get; private set; }

        public override bool TryReadFromStream(SafeBinaryReader reader)
        {
            AccelerationX = reader.ReadInt32();
            AccelerationY = reader.ReadInt32();
            AccelerationZ = reader.ReadInt32();

            return true;
        }
    }

    [SampleType(SampleType.AngularRate)]
    public class AngularRateSample : RESDSample
    {
        public override int? Width => 4 * 3;

        public int AngularRateX { get; private set; }
        public int AngularRateY { get; private set; }
        public int AngularRateZ { get; private set; }

        public override bool TryReadFromStream(SafeBinaryReader reader)
        {
            AngularRateX = reader.ReadInt32();
            AngularRateY = reader.ReadInt32();
            AngularRateZ = reader.ReadInt32();

            return true;
        }
    }

    [SampleType(SampleType.Voltage)]
    public class VoltageSample : RESDSample
    {
        public override int? Width => 4;

        public uint Voltage { get; private set; }

        public override bool TryReadFromStream(SafeBinaryReader reader)
        {
            Voltage = reader.ReadUInt32();

            return true;
        }
    }

    [SampleType(SampleType.ECG)]
    public class ECGSample : RESDSample
    {
        public override int? Width => 4;

        public int ECG { get; private set; }

        public override bool TryReadFromStream(SafeBinaryReader reader)
        {
            ECG = reader.ReadInt32();

            return true;
        }
    }

    public class SampleTypeAttribute : Attribute
    {
        public SampleTypeAttribute(SampleType sampleType)
        {
            this.SampleType = sampleType;
        }

        public SampleType SampleType { get; }
    }

    public enum SampleType
    {
        // General sample types
        Temperature = 0x0001,
        Acceleration = 0x002,
        AngularRate = 0x0003,
        Voltage = 0x0004,
        ECG = 0x0005,

        // Custom sample types
        Custom = 0xF000,
    }
}
