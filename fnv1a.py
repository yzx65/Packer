def fnv1a(s):
	h = 0x811c9dc5
	for i in s:
		h ^= ord(i)
		h *= 0x01000193
		h &= 0xFFFFFFFF
	return h
