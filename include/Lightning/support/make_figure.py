import matplotlib.pyplot as plt
import numpy as np

import pathlib


def get_cstring(file):
    """
    Read a null-terminated string from a file.
    :param file: Handle to the file to read from.
    :return: A potentially empty string read from the file.
    """
    byte_array = bytearray()
    while True:
        byte_s = file.read(1)
        if byte_s == b'\x00':
            break
        byte_array += byte_s
    return byte_array.decode('utf-8')


def make_image(file_path_in: str, save_fig_dir: str) -> bool:
    """
    Read the image data from a file and create the requested plot and save it at the requested path, relative to the
    save_fig_dir.

    :param file_path_in: The path to the file containing the image data.
    :param save_fig_dir: The directory to save the figure in.
    :return: Returns whether the image was successfully created and saved.
    """
    with open(file_path_in, 'rb') as f:
        arguments = {}

        # Get the save path.
        byte_s = f.read(1).decode('utf-8')
        assert (byte_s == 's')
        # Save the plot.
        local_path = get_cstring(f)

        # Read the dimensions of the plot.
        byte_s = f.read(1).decode('utf-8')
        assert (byte_s == 'D')
        x, y = np.fromfile(f, dtype=np.float64, count=2)
        plt.figure(figsize=(x, y))

        # Potentially get labels and title.
        byte_s = f.read(1).decode('utf-8')
        while byte_s:
            if byte_s == 'X':
                # Add a x-axis label.
                label = get_cstring(f)
                if 0 < len(label):
                    plt.xlabel(label)
            elif byte_s == 'Y':
                # Add a y-axis label.
                label = get_cstring(f)
                if 0 < len(label):
                    plt.ylabel(label)
            elif byte_s == 'T':
                # Set title.
                label = get_cstring(f)
                if 0 < len(label):
                    plt.title(label)
            else:
                # End of labels and title.
                break
            # Read next byte.
            byte_s = f.read(1).decode('utf-8')

        # Get plotting actions.
        while byte_s:
            if byte_s == 'P':
                # Plot a line.
                # Get the amount of data to expect and the data type.
                size = np.fromfile(f, dtype=np.uint64, count=1)
                x = np.fromfile(f, dtype=np.float64, count=size)
                y = np.fromfile(f, dtype=np.float64, count=size)
                label = get_cstring(f)
                if 0 < len(label):
                    plt.plot(x, y, label=label, **arguments)
                else:
                    plt.plot(x, y, **arguments)
            elif byte_s == 'S':
                # Scatter plot.
                # Get the amount of data to expect and the data type.
                size = np.fromfile(f, dtype=np.uint64, count=1)[0]
                x = np.fromfile(f, dtype=np.float64, count=size)
                y = np.fromfile(f, dtype=np.float64, count=size)
                label = get_cstring(f)
                if 0 < len(label):
                    plt.scatter(x, y, label=label, **arguments)
                else:
                    plt.scatter(x, y, **arguments)
            elif byte_s == 'E':
                # Error bars plot
                # Get the amount of data to expect and the data type.
                size = np.fromfile(f, dtype=np.uint64, count=1)[0]
                x = np.fromfile(f, dtype=np.float64, count=size)
                y = np.fromfile(f, dtype=np.float64, count=size)
                yerr = np.fromfile(f, dtype=np.float64, count=size)
                label = get_cstring(f)
                if 0 < len(label):
                    plt.errorbar(x, y, yerr=yerr, label=label, **arguments)
                else:
                    plt.errorbar(x, y, yerr=yerr, **arguments)
            elif byte_s == 'O':
                # Add an option.
                label = get_cstring(f)
                o_type = f.read(1).decode('utf-8')
                if o_type == 'S':
                    # String
                    x = get_cstring(f)
                elif o_type == 'I':
                    # Integer
                    x = int.from_bytes(f.read(4), byteorder='little')
                elif o_type == 'D':
                    # Float
                    x = np.fromfile(f, dtype=np.float64, count=1)[0]
                else:
                    # Invalid type.
                    return False
                arguments[label] = x
            elif byte_s == 'R':
                # Reset options.
                arguments = {}
            else:
                # Invalid byte.
                return False

            # Read next byte.
            byte_s = f.read(1).decode('utf-8')

    # Save the figure.
    if 0 < len(local_path):
        path = pathlib.Path(local_path)
        # Create any directories that don't exist.
        path.parent.mkdir(parents=True, exist_ok=True)
        plt.legend()
        plt.savefig(save_fig_dir / path)
        return True
    return False
