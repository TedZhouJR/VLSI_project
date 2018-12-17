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
    blocks.remove([''])

    print("data loaded, start to paint...")

    root = Tk()
    root.title('Canvas')
    canvas = Canvas(root, width=1000, height=1000, bg='white')
    for block in blocks:
        canvas.create_rectangle(int(block[0]), int(block[1]), int(block[0]) + int(block[2]), int(block[1]) + int(block[3]), fill='blue')
    canvas.pack()
    print("visualization finished")
    root.mainloop()