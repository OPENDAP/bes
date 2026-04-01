import socket

def get_hostname(ip_address):
  """
  Attempts to retrieve the hostname for a given IP address using reverse DNS lookup.

  Args:
      ip_address: The IP address to get the hostname for (string).

  Returns:
      The hostname if found, otherwise returns the original IP address (string).
  """
  try:
    hostname = socket.gethostbyaddr(ip_address)[0]
    return hostname
  except socket.herror:
    # Handle potential errors during lookup
    return ip_address

def process_file(filename):
  """
  Reads an input file with lines containing count numbers and IP addresses,
  attempts reverse DNS lookup for IPs, and prints the results.

  Args:
      filename: The name of the text file to process (string).
  """
  with open(filename, 'r') as file:
    for line in file:
      # Split the line based on whitespace (assuming port is first)
      count, ip_address = line.strip().split()
      hostname = get_hostname(ip_address)
      print(f"Count: {count}, IP: {ip_address}, Hostname: {hostname}")

# Replace 'ip_list.txt' with your actual filename
process_file('accesses.06.26.24.txt')
