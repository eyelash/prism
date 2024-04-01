# single line comment

def strings():
    "hello"
    'hello'
    """hello"""
    '''hello'''

def numbers():
    # integers
    1
    0x1
    0b1
    0o1

    # floating-point
    1.2
    .2
    1.
    1.2e3
    .2e3
    1.e3
    1e3
    1e+3
    1e-3

    # separators
    1_2

def control_flow(b):
    if b:
        return
    else:
        for i in range(10):
            continue
        while b:
            break
