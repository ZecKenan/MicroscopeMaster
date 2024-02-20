from cellpose import models, plot
from cellpose.io import imread
import os
import numpy as np
import matplotlib.pyplot as plt
from skimage import io
from alive_progress import alive_bar
import sys

class ImageSegmenter:
    def __init__(self, image_dir, model_path, diam=0):

        try:
            # Check if the image directory exists
            if not os.path.exists(image_dir) or not os.path.isdir(image_dir):
                raise ValueError(f"The specified image directory '{image_dir}' "
                                 f"does not exist or is not a valid directory.")

            # Check if the model file exists
            if not os.path.exists(model_path) or not os.path.isfile(model_path):
                raise ValueError(f"The specified model file '{model_path}' "
                                 f"does not exist or is not a valid file.")
        except ValueError as e:
            print(e)
            sys.exit(1)

        self.image_dir = image_dir
        self.model_path = model_path
        self.diam = diam

    # Asks the user wants to save the segmentation, create a directory inside
    # the image directory and save segmentation there. Otherwise, just display.
    def ask_user_to_save_images(self, segmentation_results, images):
        user_input = input(f"Do you want to save the segmented images? "
                        f"If you choose 'y', the images will be saved into a folder "
                        f"called 'Segmented Images' in your image directory. "
                        f"Otherwise, they will be displayed here. "
                        f"Save images? (y/n): ")
        if user_input.lower().strip() == 'y':
            # If yes - they are viewed in the map.
            self.save_segmented_images(segmentation_results, images)
        else:
            # Otherwise they are displayed here
            self.display_segmented_images(segmentation_results, images)

    # Function to plot the segmentation.
    def plot_segmentation(self, image_file, masks, flows, images):

        maski = masks #param in show_segmentation
        flowi = flows[0] #param in show_segmentation

        # Create a figure for plotting
        fig = plt.figure(figsize=(12, 10))

        plot.show_segmentation(fig, images, maski, flowi)

        plt.annotate(f"Segmentation of {image_file}", (-1.2, 1.9),
                    xycoords='axes fraction',
                    fontsize=12, ha='center')

    # Checks if there is a directory called "Segmented Images". If not, creates
    # one and saves the images there. If it does, overwrite.
    def save_segmented_images(self, segmentation_results, images):
        save_dir = os.path.join(self.image_dir, "Segmented Images")
        os.makedirs(save_dir, exist_ok=True)

        for i, (image_file, masks, flows, styles) in enumerate(zip(*segmentation_results)):
            # Call the plot segmentation function
            self.plot_segmentation(image_file, masks, flows, images)

            # Save the plotted image
            save_path = os.path.join(save_dir, f"segmented_{i+1}.png")
            plt.savefig(save_path)
            plt.close()
            print(f"Segmented image saved in: {save_path}")

    # Function for displaying the segmentation if the user don't want to save
    # them to a file.
    def display_segmented_images(self, segmentation_results, images):
        for i, (image_file, masks, flows, styles) in enumerate(zip(*segmentation_results)):
            # Call the plotting function.
            self.plot_segmentation(image_file, masks, flows, images)
            # Display plot.
            plt.show()

    def segmentation(self):
        # Create a list of IMAGES in the image directory. This is used as input
        # source in the segmentation.
        Image_files = [f for f in os.listdir(self.image_dir) if f.endswith(('.jpg',
                                                                '.png', '.tif'))]

        # Load the specific model
        model = models.CellposeModel(gpu=False, pretrained_model=self.model_path)

        # For counting no. of segmented images.
        image_count = 0
        # Init. list to store segmentation information.
        masks_list = []
        flows_list = []
        styles_list = []

        # Nested function: Check number of images to segment. If > 1, alert that
        # it will take time. Throw exception if user don't want to continue.
        def check_image_count(image_dir, threshold=1):
            image_files = [f for f in os.listdir(image_dir) if f.endswith(('.jpg',
                                                                '.png', '.tif'))]
            num_images = len(image_files)

            # Alert user. Many images can take long time.
            if num_images > threshold:
                user_input = input(f"There are multiple images in the folder."
                            f"This will take approx {num_images} min. to segment"
                            f" without a GPU. Do you want to continue? (y/n): ")
                try:
                    if user_input.lower().strip() != 'y':
                        raise ValueError("Segmentation aborted. ")
                except ValueError as e:
                    print(e)
                    sys.exit(1)
                else:
                    print("Continuing...")
        check_image_count(self.image_dir)

        # Define a custom sorting key function to bypass the different sorting
        # approaches of different operating systems. Sorts numerically.
        def sort_key(filename):
            # Extract the numeric part of the filename (e.g., "1" from "pic1.jpg")
            numeric_part = int(''.join(filter(str.isdigit, filename)))

            return numeric_part

        Image_files = sorted(Image_files, key=sort_key)

        # Create a list of indices in ascending numerical order
        indices = list(range(len(Image_files)))

        # Use alive_bar for progress tracking
        with alive_bar(len(indices), title="Segmenting Images") as progress_bar:
            for index in indices:
                image_file = Image_files[index]
                # Construct the full path to the image file
                image_path = os.path.join(self.image_dir, image_file)

                # Load the image
                images = io.imread(image_path)

                # Perform segmentation
                masks, flows, styles = model.eval(images, diameter=self.diam,
                                        channels=[0, 0], flow_threshold=0.4,
                                        do_3D=False)
                image_count += 1

                # Append the segmented results to the respective lists
                masks_list.append(masks)
                flows_list.append(flows)
                styles_list.append(styles)

                # Update the progress bar
                progress_bar()

        # Ask the user if they want to save or display segmented images
        self.ask_user_to_save_images((Image_files, masks_list, flows_list, styles_list), images)

        return (Image_files, masks_list, flows_list, styles_list), image_count
####                                                                        ####
###                                                                          ###
##                                                                            ##
#                                                                              #
# Instructions:
# Follow these steps to segment the images taken with the microscope:
# 1. Download the pretrained model.
# 2. Create an instance of the ImageSegmenter class. Use the following variables:
#    - image_dir = (absolut) path to your image directory
#    - model_path = (absolut) path to the pretrained model.
#    - diam = (Optionally) provide cell diameter (int). Defaul = '0' (the
#              algorithm estimates the diameter itself).
# 3. Call the segmentation method on the instance.
# 4. Follow prompts to decide how and where to wiev results. choosing 'y' on
#    'do you want to save the image' creates a directory called 'Segmented
#    images' inside the provided image directory. The results are here. Choosing
#    'n' displays the results in the program.
# 5. The script segments the images, display the results and specifies the
#    number of segmented images. Every segmented image have the original image,
#    the masks and the predicted flow (vector field, see thesis 2.5.2 Cellpose).
#                                                                              #
##                                                                            ##
###                                                                          ###
####                                                                        ####
# Example:
#Create an instance of the ImageSegmenter class
segmenter = ImageSegmenter(image_dir="/Users/zeeko/Desktop/Try/ForCodeCopy",
                           model_path="/Users/zeeko/Desktop/Try/fourteenthCopy")

# Call the segmentation method on the instance
segmentation_results, processed_image_count = segmenter.segmentation()

# Print the result
print(f"Number of segmented images: {processed_image_count}")
