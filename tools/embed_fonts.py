import pathlib
import sys


source = pathlib.Path(sys.argv[1])
destination = pathlib.Path(sys.argv[2])
destination.parent.mkdir(parents=True, exist_ok=True)
parts = ['#pragma once\nnamespace toolscreen::assets {\n']
for name, symbol in [('Regular', 'interRegular'), ('Medium', 'interMedium'), ('SemiBold', 'interSemiBold')]:
    data = (source / ('Inter-' + name + '.ttf')).read_bytes()
    parts.append('inline constexpr unsigned char ' + symbol + '[] = {\n')
    parts.extend(','.join(str(value) for value in data[offset:offset + 32]) + ',\n' for offset in range(0, len(data), 32))
    parts.append('};\n')
for filename, symbol in [('JetBrainsMono-Regular.ttf', 'jetBrainsMono'), ('Monocraft.ttf', 'monocraft')]:
    data = (source / filename).read_bytes()
    parts.append('inline constexpr unsigned char ' + symbol + '[] = {\n')
    parts.extend(','.join(str(value) for value in data[offset:offset + 32]) + ',\n' for offset in range(0, len(data), 32))
    parts.append('};\n')
parts.append('}\n')
destination.write_text(''.join(parts))
