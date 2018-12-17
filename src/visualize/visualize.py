from tkinter import *

file_path = ""
file_name = "sample.txt"

with open(file_path + file_name, 'r') as f:
    print("visualization starting...")
    blocks = []
    line = f.readline()
    blocks.append(line[:-1].split(' '))

    while line:
        line = f.readline()
        blocks.append(line[:-1].split(' '))

    if [''] in blocks:
        blocks.remove([''])

    print("data loaded, start to paint...")

    root = Tk()
    root.title('Canvas')
    max_x = max(((int)(x[0]) + (int)(x[2])) for x in blocks)
    max_y = max(((int)(x[1]) + (int)(x[3])) for x in blocks)
    if max_x > max_y and max_x > 1000:
        ratio = max_x / 1500.0
    elif max_x <= max_y and max_y > 1000:
        ratio = max_y / 1500.0
    else:
        ratio = 1.0
    canvas = Canvas(root, width=max_x, height=max_y, bg='white')
    for block in blocks:
        print(int(block[0]), int(block[1]), int(block[0]) + int(block[2]), int(block[1]) + int(block[3]))
        canvas.create_rectangle(int(block[0]) / ratio, int(block[1]) / ratio, int(block[0]) / ratio + int(block[2]) / ratio,
                                int(block[1]) / ratio + int(block[3]) / ratio, fill='blue')
    canvas.pack()
    print("visualization finished")
    root.mainloop()