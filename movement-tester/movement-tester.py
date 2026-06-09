import math
import os
import pathlib
import socket
import time
from dataclasses import dataclass
from typing import List

if any(not os.path.exists(proto + '_pb2.py') for proto in
       ('messages_robocup_ssl_detection_tracked', 'messages_robocup_ssl_wrapper_tracked')):
    print("Compiling Protobuf files...")
    import grpc_tools.protoc

    grpc_tools.protoc.main([
        'protoc',
        '--python_out=.', '--pyi_out=.',
        *[str(path) for path in pathlib.Path().rglob('*.proto')]
    ])

from messages_robocup_ssl_detection_tracked_pb2 import TrackedBall
from messages_robocup_ssl_wrapper_tracked_pb2 import TrackerWrapperPacket


@dataclass(frozen=True)
class Vec2:
    x: float
    y: float

    def __add__(self, other):
        return Vec2(self.x + other.x, self.y + other.y)

    def __sub__(self, other: "Vec2"):
        return Vec2(self.x - other.x, self.y - other.y)

    def __mul__(self, factor: float):
        return Vec2(self.x * factor, self.y * factor)

    def __truediv__(self, divisor: float):
        return Vec2(self.x / divisor, self.y / divisor)


@dataclass(frozen=True)
class Movement:
    target_pos: Vec2
    duration: float


########################################################################################################################
# CONFIG AREA
INTERFACE = "10.0.0.121"  # Specify interface here, like "eth0", "192.168.0.42", etc.
DESTINATION_ADDRESS = ("224.5.23.2", 10010)
OUTPUT_FREQUENCY = 60
MOVEMENTS = [
    Movement(target_pos=Vec2(0, 0), duration=2),
    Movement(target_pos=Vec2(-6, -4.5), duration=2),
    Movement(target_pos=Vec2(6, -4.5), duration=2),
    Movement(target_pos=Vec2(6, 4.5), duration=2),
    Movement(target_pos=Vec2(-6, 4.5), duration=2),
]
INTERPOLATE_MOVEMENTS = True


########################################################################################################################

class Publisher:
    sock: socket.socket
    frame_number: int

    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        self.sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)
        self.sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(INTERFACE))
        self.frame_number = 0

    def publish_tracked_ball(self, pos: Vec2, vel: Vec2):
        wrapped = TrackerWrapperPacket()
        wrapped.uuid = "TIGERs Mannheim gimbal-rpi movement-tester"
        wrapped.source_name = "TIGERs"
        frame = wrapped.tracked_frame

        ball = TrackedBall()
        ball.pos.x = pos.x
        ball.pos.y = pos.y
        ball.pos.z = 0
        ball.vel.x = vel.x
        ball.vel.y = vel.y
        ball.vel.z = 0
        ball.visibility = 1

        frame.frame_number = self.frame_number
        frame.timestamp = time.time()
        frame.balls.append(ball)

        self.frame_number += 1
        self.sock.sendto(wrapped.SerializeToString(), DESTINATION_ADDRESS)

        print(f"Ball at {pos.x: .2f}, {pos.y: .2f} m with {math.sqrt(vel.x ** 2 + vel.y ** 2):.2f} m/s")


class MovementTester:
    pub: Publisher
    movements: List[Movement]
    current_start_time: float
    current_pos: Vec2

    def __init__(self):
        self.pub = Publisher()
        self.current_pos = Vec2(0, 0)
        self.movements = []
        self.current_pos = Vec2(0, 0)

    def spin_once(self):
        t_now = time.time()

        if len(self.movements) == 0:
            self.movements = list(MOVEMENTS)
            self.current_start_time = t_now

        t_remaining = self.movements[0].duration - (t_now - self.current_start_time)
        vel: Vec2
        if t_remaining < 0:
            self.current_pos = self.movements[0].target_pos
            vel = Vec2(0, 0)
            self.movements = self.movements[1:]
            self.current_start_time = t_now

        elif INTERPOLATE_MOVEMENTS:
            diff = self.movements[0].target_pos - self.current_pos
            vel = diff / t_remaining
            self.current_pos += vel * (1 / OUTPUT_FREQUENCY)

        else:
            self.current_pos = self.movements[0].target_pos
            vel = Vec2(0, 0)

        self.pub.publish_tracked_ball(self.current_pos, vel)


if __name__ == '__main__':
    assert len(MOVEMENTS) > 0, "You need to specify at least one movement using the MOVEMENTS variable"

    tester = MovementTester()
    while True:
        tester.spin_once()
        time.sleep(1 / OUTPUT_FREQUENCY)
