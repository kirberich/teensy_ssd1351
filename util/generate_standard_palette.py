from pprint import pprint

colors = []
for r in range(0, 8):
    for g in range(0, 8):
        for b in range(0, 4):
            colors.append([
                int(round(r * 36.428571429)),
                int(round(g * 36.428571429)),
                b * 85
            ])

pprint(colors)
print(len(colors))

# Create a simple html overview of the palette
with open('palette.html', 'w') as f:
    f.write('<html><body>')
    for i, c in enumerate(colors):
        if i and not (i % 32):
            f.write('<br/>\n')
        f.write('<span style="background-color:rgb({r},{g},{b});width:30px;height:20px;margin:0;display:inline-block;">{i}</span>\n'.format(
            r=c[0], g=c[1], b=c[2], i=i)
        )

    f.write('<br/><br/>\n')

    for i, c in enumerate(colors):
        f.write('<div style="background-color:rgb({r}, {g}, {b})">{i}: {r}, {g}, {b}</div>\n'.format(r=c[0], g=c[1], b=c[2], i=i))

    f.write('</body></html>')

with open('../standard_palette.h', 'w') as f:
    f.write('#include "color.h"\n\n')
    f.write('static const Palette standard_palette = {\n');
    f.write(",\n".join(['\t{{{r}, {r}, {b}}}'.format(r=c[0], g=c[1], b=c[2]) for c in colors]))
    f.write("\n};")
