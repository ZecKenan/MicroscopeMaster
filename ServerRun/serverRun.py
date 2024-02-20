from flask import Flask, request
import os

app = Flask(__name__)

# Initialize a counter to keep track of the image number
image_counter = 0

@app.route('/upload', methods=['POST'])
def upload_image():
    global image_counter  # Declare image_counter as a global variable
    image_data = request.data

    # Generate the filename with the current image number
    filename = '/Users/zeeko/Desktop/ImageAcquistion/pic{}.jpg'.format(image_counter)

    # Save the image with the generated filename
    with open(filename, 'wb') as f:
        f.write(image_data)

    # Increment the image counter for the next image
    image_counter += 1

    print("Image successfully saved to: ", filename)
    return 'Image received and saved successfully!'

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000)
