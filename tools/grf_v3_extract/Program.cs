using System;
using System.IO;
using System.IO.Compression;
using System.Text;
using System.Text.RegularExpressions;

internal static class Program
{
    private static byte[] ExpandZlib(byte[] packed, int expectedSize)
    {
        if (packed.Length < 6)
            throw new InvalidDataException("Invalid zlib stream.");
        using (var input = new MemoryStream(packed, 2, packed.Length - 6, false))
        using (var deflate = new DeflateStream(input, CompressionMode.Decompress))
        using (var output = new MemoryStream(expectedSize))
        {
            deflate.CopyTo(output);
            return output.ToArray();
        }
    }

    private static int Main(string[] args)
    {
        if (args.Length < 2)
        {
            Console.Error.WriteLine("Usage: grf_v3_extract <archive.grf> <regex> [output-directory]");
            return 2;
        }

        var archivePath = Path.GetFullPath(args[0]);
        var wanted = new Regex(args[1], RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);
        var outputDirectory = args.Length >= 3 ? Path.GetFullPath(args[2]) : null;

        using (var archive = File.OpenRead(archivePath))
        using (var reader = new BinaryReader(archive, Encoding.GetEncoding(28591), true))
        {
            var magic = Encoding.ASCII.GetString(reader.ReadBytes(16)).TrimEnd('\0');
            reader.ReadBytes(14);

            ulong tableOffset;
            uint fileCount;
            if (magic.StartsWith("Master of Magic", StringComparison.Ordinal))
            {
                tableOffset = reader.ReadUInt32();
                var seed = reader.ReadUInt32();
                fileCount = reader.ReadUInt32() - seed - 7;
            }
            else if (magic.StartsWith("Event Horizon", StringComparison.Ordinal))
            {
                tableOffset = reader.ReadUInt64() + 4;
                fileCount = reader.ReadUInt32();
            }
            else
            {
                throw new InvalidDataException("Unsupported GRF signature.");
            }
            var version = reader.ReadUInt32();
            if (version != 0x200 && version != 0x300)
                throw new InvalidDataException(String.Format("Unsupported GRF version: 0x{0:X}.", version));

            archive.Seek((long)tableOffset, SeekOrigin.Current);
            var compressedTableSize = reader.ReadUInt32();
            var tableSize = reader.ReadUInt32();
            var table = ExpandZlib(reader.ReadBytes(checked((int)compressedTableSize)), checked((int)tableSize));
            if (table.Length != tableSize)
                throw new InvalidDataException("GRF table size mismatch.");

            var position = 0;
            var matched = 0;
            for (uint i = 0; i < fileCount; i++)
            {
                var nameEnd = Array.IndexOf(table, (byte)0, position);
                if (nameEnd < 0)
                    throw new InvalidDataException("Invalid GRF table.");
                var name = Encoding.GetEncoding(28591).GetString(table, position, nameEnd - position);
                position = nameEnd + 1;

                var compressedSize = BitConverter.ToUInt32(table, position); position += 4;
                var alignedSize = BitConverter.ToUInt32(table, position); position += 4;
                var size = BitConverter.ToUInt32(table, position); position += 4;
                var type = table[position++];
                ulong offset;
                if (version == 0x300)
                {
                    offset = BitConverter.ToUInt64(table, position); position += 8;
                }
                else
                {
                    offset = BitConverter.ToUInt32(table, position); position += 4;
                }

                if ((type & 1) == 0 || !wanted.IsMatch(name))
                    continue;

                matched++;
                Console.WriteLine("{0}\tsize={1}\tpacked={2}\ttype={3}\toffset={4}", name, size, compressedSize, type, offset);
                if (outputDirectory == null)
                    continue;
                if ((type & 6) != 0)
                {
                    Console.Error.WriteLine("Skipping encrypted entry: {0} (type {1})", name, type);
                    continue;
                }

                archive.Seek(46 + checked((long)offset), SeekOrigin.Begin);
                var packed = reader.ReadBytes(checked((int)compressedSize));
                var contents = compressedSize == size ? packed : ExpandZlib(packed, checked((int)size));
                var relative = name.Replace('\\', Path.DirectorySeparatorChar);
                var target = Path.Combine(outputDirectory, relative);
                Directory.CreateDirectory(Path.GetDirectoryName(target));
                File.WriteAllBytes(target, contents);
            }

            Console.Error.WriteLine("Signature={0}; Version=0x{1:X}; Files={2}; Matches={3}", magic, version, fileCount, matched);
        }
        return 0;
    }
}
