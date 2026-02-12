#numbers = [1, 2, 3, 4, 5]
numbers = list(range(48))
with open("lnumbers.bin", "wb") as file:
    for number in numbers:
        single_byte=bytes([number])
        file.write(single_byte)  # Adjust byte size as needed
        #file.write(number.to_bytes(1, byteorder='little'))  # Adjust byte size as needed

