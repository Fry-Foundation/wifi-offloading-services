# generate valid ones and print lines!!

#!/bin/sh

# Number of key pairs to generate
NUM_KEYS=5

# Directory to store the key pairs
KEY_DIR="ed25519_keys"
mkdir -p $KEY_DIR

# Function to validate Base64 encoding
validate_base64() {
  echo "$1" | openssl base64 -d &> /dev/null
  return $?
}

# Generate key pairs
for i in $(seq 1 $NUM_KEYS); do
  # Generate the private key
  openssl genpkey -algorithm ed25519 -out $KEY_DIR/private_key_$i.pem

  # Extract and encode the public key in Base64 without headers
  PUB_KEY_BASE64=$(openssl pkey -in $KEY_DIR/private_key_$i.pem -pubout -outform DER | openssl base64 -A)

  # Validate the Base64 encoding
  if validate_base64 "$PUB_KEY_BASE64"; then
    echo "$PUB_KEY_BASE64"
  else
    echo "Invalid Base64 Encoding for Key $i"
  fi

done


# generate valid ones!

#!/bin/sh

# Number of key pairs to generate
NUM_KEYS=20

# Directory to store the key pairs
KEY_DIR="ed25519_keys"
mkdir -p $KEY_DIR

# Function to validate Base64 encoding
validate_base64() {
  echo "$1" | openssl base64 -d &> /dev/null
  return $?
}

# Generate key pairs
for i in $(seq 1 $NUM_KEYS); do
  # Generate the private key
  openssl genpkey -algorithm ed25519 -out $KEY_DIR/private_key_$i.pem

  # Extract the public key from the private key
  openssl pkey -in $KEY_DIR/private_key_$i.pem -pubout -out $KEY_DIR/public_key_$i.pem

  # Extract and encode the public key in Base64
  PUB_KEY_BASE64=$(openssl pkey -in $KEY_DIR/private_key_$i.pem -pubout -outform DER | openssl base64)

  # Validate the Base64 encoding
  if validate_base64 "$PUB_KEY_BASE64"; then
    echo "Public Key $i (Valid):"
    echo "$PUB_KEY_BASE64"
  else
    echo "Public Key $i (Invalid Base64 Encoding):"
    echo "$PUB_KEY_BASE64"
  fi

  echo "---------------------"
done
