import os
from PIL import Image

# Define the folder path
folder_path = '.'

# Define the target size for the thumbnails
thumbnail_size = (128, 128)

# Iterate over all files in the folder
for filename in os.listdir(folder_path):
    # Check if the file is a DDS file
    if filename.lower().endswith('.dds'):
        file_path = os.path.join(folder_path, filename)
        
        # Delete files with "thumb" in their name
        if 'thumb' in filename.lower():
            os.remove(file_path)
            print(f"Deleted: {filename}")
            continue
        
        # Open the DDS file
        try:
            with Image.open(file_path) as img:
                # Convert to PNG and resize
                img = img.convert('RGBA')
                img.thumbnail(thumbnail_size)
                
                # Create the new filename with "thumb" appended
                new_filename = os.path.splitext(filename)[0] + '_thumb.png'
                new_file_path = os.path.join(folder_path, new_filename)
                
                # Save the resized image as PNG
                img.save(new_file_path, 'PNG')
                print(f"Converted and resized: {filename} -> {new_filename}")
                os.remove(file_path)
        
        except Exception as e:
            print(f"Failed to process {filename}: {e}")

print("Processing complete.")