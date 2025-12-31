#!/usr/bin/env python3
"""
Extract reference display from test suite PNG images and convert to PBM format.

The test suite images are 768x384 (64x32 scaled by 12x).
We sample the center of each 12x12 block to get the CHIP-8 pixel state.
"""

import sys
from PIL import Image

def png_to_pbm(png_path: str, pbm_path: str, scale: int = 12):
    """Convert a scaled-up CHIP-8 screenshot to PBM format."""
    img = Image.open(png_path).convert('RGBA')
    
    width, height = img.size
    chip8_width = width // scale
    chip8_height = height // scale
    
    print(f"Image size: {width}x{height}")
    print(f"CHIP-8 size: {chip8_width}x{chip8_height}")
    print(f"Scale factor: {scale}x")
    
    if chip8_width != 64 or chip8_height != 32:
        print(f"Warning: Expected 64x32 CHIP-8 display, got {chip8_width}x{chip8_height}")
    
    # Extract pixels by sampling center of each block
    pixels = []
    for y in range(chip8_height):
        row = []
        for x in range(chip8_width):
            # Sample center of the block
            px = x * scale + scale // 2
            py = y * scale + scale // 2
            r, g, b, a = img.getpixel((px, py))
            
            # Determine if pixel is "on" (bright) or "off" (dark)
            # CHIP-8 typically uses light pixels on dark background
            brightness = (r + g + b) / 3
            pixel_on = brightness > 128
            row.append('1' if pixel_on else '0')
        pixels.append(row)
    
    # Write PBM file
    with open(pbm_path, 'w') as f:
        f.write("P1\n")
        f.write(f"# Reference from {png_path}\n")
        f.write(f"{chip8_width} {chip8_height}\n")
        for row in pixels:
            f.write(' '.join(row) + '\n')
    
    print(f"Wrote {pbm_path}")
    
    # Print ASCII preview
    print("\nASCII preview:")
    for row in pixels:
        print(''.join('█' if p == '1' else '.' for p in row))

def main():
    if len(sys.argv) == 2 and sys.argv[1] == '--all':
        # Convert all standard reference images
        import os
        base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        pics_dir = os.path.join(base_dir, 'tests', 'chip8-test-suite', 'pictures')
        refs_dir = os.path.join(base_dir, 'tests', 'references')
        
        os.makedirs(refs_dir, exist_ok=True)
        
        conversions = [
            ('chip-8-logo.png', '1-chip8-logo.pbm'),
            ('ibm-logo.png', '2-ibm-logo.pbm'),
            ('corax+.png', '3-corax.pbm'),
            ('flags.png', '4-flags.pbm'),
        ]
        
        for png_name, pbm_name in conversions:
            png_path = os.path.join(pics_dir, png_name)
            pbm_path = os.path.join(refs_dir, pbm_name)
            print(f"\n{'='*60}")
            print(f"Converting {png_name} -> {pbm_name}")
            print('='*60)
            png_to_pbm(png_path, pbm_path)
    elif len(sys.argv) >= 3:
        png_to_pbm(sys.argv[1], sys.argv[2])
    else:
        print(f"Usage: {sys.argv[0]} <input.png> <output.pbm>")
        print(f"       {sys.argv[0]} --all")
        sys.exit(1)

if __name__ == '__main__':
    main()
