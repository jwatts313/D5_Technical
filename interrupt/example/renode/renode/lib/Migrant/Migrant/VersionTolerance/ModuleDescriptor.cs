//
// Copyright (c) 2012-2021 Antmicro
//
// This file is licensed under the MIT License.
// Full license text is available in the LICENSE file.

using System;
using System.Reflection;
using Antmicro.Migrant.Customization;
using Antmicro.Migrant.Utilities;
using System.Diagnostics;

namespace Antmicro.Migrant.VersionTolerance
{
    [DebuggerDisplay("Name = {Name}, GUID = {GUID}")]
    internal class ModuleDescriptor : IIdentifiedElement
    {
        public ModuleDescriptor()
        {
        }

        public ModuleDescriptor(Module module)
        {
            ModuleAssembly = new AssemblyDescriptor(module.Assembly);
            Name = module.Name;
            GUID = module.ModuleVersionId;
        }

        public void Read(ObjectReader reader)
        {
            ModuleAssembly = reader.Assemblies.Read();
            GUID = reader.PrimitiveReader.ReadGuid();
            Name = reader.PrimitiveReader.ReadString();
        }

        public void Write(ObjectWriter writer)
        {
            writer.Assemblies.TouchAndWriteId(ModuleAssembly);
            writer.PrimitiveWriter.Write(GUID);
            writer.PrimitiveWriter.Write(Name);
        }

        public override bool Equals(object obj)
        {
            if(obj == null || obj.GetType() != typeof(ModuleDescriptor))
            {
                return false;
            }
            if(ReferenceEquals(this, obj))
            {
                return true;
            }
            ModuleDescriptor other = (ModuleDescriptor)obj;
            return Name == other.Name && GUID == other.GUID && ModuleAssembly.Equals(other.ModuleAssembly);
        }
        

        public override int GetHashCode()
        {
            unchecked
            {
                return (Name != null ? Name.GetHashCode() : 0) ^ GUID.GetHashCode() ^ (ModuleAssembly != null ? ModuleAssembly.GetHashCode() : 0);
            }
        }
        

        public bool Equals(ModuleDescriptor obj, VersionToleranceLevel versionToleranceLevel)
        {
            if(versionToleranceLevel.HasFlag(VersionToleranceLevel.AllowGuidChange))
            {
                return obj != null && obj.Name == Name && ModuleAssembly.Equals(obj.ModuleAssembly, versionToleranceLevel);
            }
            return obj != null && obj.GUID == GUID && ModuleAssembly.Equals(obj.ModuleAssembly, versionToleranceLevel);
        }

        public string Name { get; private set; }

        public Guid GUID { get; private set; } 

        public AssemblyDescriptor ModuleAssembly { get; private set; }
    }
}

