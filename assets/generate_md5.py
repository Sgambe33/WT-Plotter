from PIL import Image

def dhash_from_image(image: Image.Image, size: int = 9) -> str:
    """
    Calculates the dHash (Difference Hash) of an image, matching the C++ logic.
    
    Args:
        image: PIL Image object.
        size: The width to resize to (default 9 creates an 8x8 hash).
        
    Returns:
        Hexadecimal string representing the hash.
    """
    # 1. Convert to Grayscale (QImage::Format_Grayscale8)
    gray = image.convert('L')

    # 2. Resize to (width=size, height=size-1)
    # Qt::IgnoreAspectRatio is the default behavior for PIL resize.
    # Qt::SmoothTransformation is similar to Resampling.LANCZOS or BICUBIC.
    small = gray.resize((size, size - 1), resample=Image.Resampling.LANCZOS)

    # 3. Compare Pixels to build bit vector
    bits = []
    pixels = small.load()  # Access pixel data for speed

    # Loop height (y from 0 to size-2)
    for y in range(small.height):
        # Loop width (x from 0 to size-2)
        for x in range(small.width - 1):
            left = pixels[x, y]
            right = pixels[x + 1, y]
            
            # C++: bits.append(left > right ? 1 : 0);
            bits.append(1 if left > right else 0)

    # 4. Convert bits to Hex String
    hex_str = ""
    
    # Process 4 bits at a time (Nibbles)
    for i in range(0, len(bits), 4):
        chunk = bits[i:i+4]
        
        # Pad with 0 if we are at the end and have fewer than 4 bits
        # (Matches C++ initialization: int nibble[4] = { 0, 0, 0, 0 };)
        while len(chunk) < 4:
            chunk.append(0)
            
        # Construct the hex value: MSB is the first bit in the chunk
        value = (chunk[0] << 3) | (chunk[1] << 2) | (chunk[2] << 1) | chunk[3]
        
        # Convert to hex character (matches QString::number(value, 16))
        hex_str += format(value, 'x')

    return hex_str

# --- Usage Example ---
if __name__ == "__main__":
    try:
        # Load an image
        img = Image.open("avg_training_ground_tankmap.png")
        
        # Calculate Hash
        result_hash = dhash_from_image(img, size=9)
        print(f"dHash: {result_hash}")
        
    except FileNotFoundError:
        print("Image file not found.")