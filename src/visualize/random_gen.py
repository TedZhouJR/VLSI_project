import random

file_path = ""
file_name = "sample.txt"

def in_block(point, block):
    return point[0] < block[0] + block[2] and point[0] > block[0] \
            and point[1] < block[1] + block[3] and point[1] > block[1]

def cross(block1, block2):
    return in_block([block1[0], block1[1]], block2) or \
           in_block([block1[0] + block1[2], block1[1]], block2) or \
           in_block([block1[0] + block1[2], block1[1] + block1[3]], block2) or \
           in_block([block1[0], block1[1] + block1[3]], block2)

amount = 800
map_size = 1000
blocks = []
for i in range(amount):
    valid = False
    new_block = []
    while not valid:
        valid = True
        new_block = [(int)(random.random() * map_size),
                     (int)(random.random() * map_size),
                     (int)(random.random() * map_size / 50),
                     (int)(random.random() * map_size / 50)]
        for block in blocks:
            if cross(new_block, block):
                valid = False
                break
    blocks.append(new_block)

with open(file_path + file_name, 'w') as f:
    for block in blocks:
        f.write(str(block[0])+' '+str(block[1])+' '+str(block[2])+' '+str(block[3])+'\n')