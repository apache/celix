
Store the AES key for encrypting and decrypting the encoded secret keys safe in a file!
Default file is `/etc/pubsub.keys` with the following format:
```
aes_key:{32 character AES key here}
aes_iv:{16 character AES iv here}
```

Use the $PROJECT_BUILD/pubsub/keygen/makecert for generating keypairs
Use the $PROJECT_BUILD/pubsub/keygen/ed_file for encrypting and decrypting private keys

Public keys need to be stored in the 'public' folder having the following format:
- pub_{topic}.pub
- sub_{topic}.pub

Secret keys need to be stored in the 'private' folder having the following format:
- pub_{topic}.key.enc
- sub_{topic}.key.enc
These files need to be encrypted using the 'ed_file' executable.
