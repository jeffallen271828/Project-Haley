from __future__ import annotations
from dataclasses import dataclass
from enum import IntEnum, IntFlag
from struct import Struct
from typing import Any
import zlib

MAGIC = 0x4841
HEADER_STRUCT = Struct("<HBBHIII")
HEADER_SIZE = HEADER_STRUCT.size

class PacketType(IntEnum):
    HEARTBEAT = 1; COMMAND = 2; ACK = 3; TELEMETRY = 4; FAULT = 5; AUDIO_META = 6; VERSION_NEGOTIATION = 7; STATE = 8

class PacketFlags(IntFlag):
    NONE = 0; REQUEST = 1 << 0; RESPONSE = 1 << 1; ACK_REQUIRED = 1 << 2; PRIORITY_HIGH = 1 << 3; EMERGENCY = 1 << 4

@dataclass(slots=True)
class PacketHeader:
    version: int; msg_type: PacketType; flags: PacketFlags; sequence: int; payload_length: int; crc32: int = 0; magic: int = MAGIC
    def to_bytes(self) -> bytes:
        return HEADER_STRUCT.pack(self.magic, self.version & 0xFF, int(self.msg_type) & 0xFF, int(self.flags) & 0xFFFF, self.sequence & 0xFFFFFFFF, self.payload_length & 0xFFFFFFFF, self.crc32 & 0xFFFFFFFF)
    @classmethod
    def from_bytes(cls, data: bytes) -> "PacketHeader":
        magic, version, msg_type, flags, sequence, payload_length, crc32 = HEADER_STRUCT.unpack_from(data)
        if magic != MAGIC: raise ValueError("bad magic")
        return cls(version=version, msg_type=PacketType(msg_type), flags=PacketFlags(flags), sequence=sequence, payload_length=payload_length, crc32=crc32, magic=magic)

@dataclass(slots=True)
class Packet:
    header: PacketHeader
    payload: bytes = b""
    def to_bytes(self) -> bytes:
        hdr = PacketHeader(self.header.version, self.header.msg_type, self.header.flags, self.header.sequence, len(self.payload), 0)
        raw = hdr.to_bytes() + self.payload
        hdr.crc32 = zlib.crc32(raw[:-4]) & 0xFFFFFFFF
        return hdr.to_bytes() + self.payload
    @classmethod
    def from_bytes(cls, data: bytes, verify_crc: bool = True) -> "Packet":
        header = PacketHeader.from_bytes(data[:HEADER_SIZE]); end = HEADER_SIZE + header.payload_length
        payload = data[HEADER_SIZE:end]
        if verify_crc:
            raw = bytearray(data[:end]); raw[-4:] = b"\x00\x00\x00\x00"
            if (zlib.crc32(bytes(raw)) & 0xFFFFFFFF) != header.crc32: raise ValueError("CRC mismatch")
        return cls(header=header, payload=payload)

def encode_packet(*, msg_type: PacketType, sequence: int, payload: bytes = b"", version: int = 1, flags: PacketFlags = PacketFlags.NONE) -> bytes:
    return Packet(PacketHeader(version, msg_type, flags, sequence, len(payload)), payload).to_bytes()

def decode_packet(data: bytes, verify_crc: bool = True) -> Packet:
    return Packet.from_bytes(data, verify_crc=verify_crc)

def make_heartbeat_packet(*, sequence: int, version: int = 1, request: bool = False) -> bytes:
    return encode_packet(msg_type=PacketType.HEARTBEAT, sequence=sequence, payload=b"", version=version, flags=PacketFlags.REQUEST if request else PacketFlags.NONE)

def encode_key_values(fields: dict[str, Any]) -> bytes:
    return ("\n".join(f"{k}={v}" for k, v in fields.items()) + "\n").encode("utf-8")

def decode_key_values(payload: bytes) -> dict[str, str]:
    out = {}
    for line in payload.decode("utf-8", errors="replace").splitlines():
        if "=" in line:
            k, v = line.split("=", 1); out[k.strip()] = v.strip()
    return out
