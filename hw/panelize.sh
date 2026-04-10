  kikit panelize \
    --layout 'grid; rows: 2; cols: 1; hspace: 3mm; vspace: 3mm;' \
    --tabs 'annotation' \
    --framing 'frame; width: 5mm; space: 3mm; cuts: h' \
    --cuts 'mousebites; drill: 0.5mm; spacing: 1mm; offset: -0.5mm; prolong: 0.5mm' \
   --source 'tolerance: 20mm' \
   --post 'millradius: 0.5mm; copperfill: true' \
    sem18.kicad_pcb panel.kicad_pcb
    
#--tabs 'fixed; width: 3mm; height: 3mm; vcount: 1; hcount: 1' \
