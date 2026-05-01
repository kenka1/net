import socket
import argparse
import selectors
import time

RECV_SIZE = 4096
TIMEOUT = 1.0


def percentile(values: list[float], p: float) -> float:
    if not values:
        return 0.0
    sorted_values = sorted(values)
    idx = int((len(sorted_values) - 1) * p)
    return sorted_values[idx]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8989)
    parser.add_argument("--freq", type=int, required=True)
    parser.add_argument("--num", type=int, required=True)
    parser.add_argument("--size", type=int, required=True)
    parser.add_argument("--log", type=str, default=None)

    args = parser.parse_args()

    latencies: list[float] = []
    recv_buf = b""
    payload = b"x" * args.size + b"\n"
    num = args.num
    delay = 1.0 / args.freq
    sent_ok = 0
    sent_err = 0
    recv_ok = 0
    recv_err = 0

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(TIMEOUT)
        sock.connect((args.host, args.port))

        sel = selectors.DefaultSelector()
        sel.register(sock, selectors.EVENT_READ | selectors.EVENT_WRITE)

        start_time = time.perf_counter()

        while num > 0:
            time_point = time.perf_counter()

            events = sel.select()
            for _, mask in events:
                if mask & selectors.EVENT_READ:
                    try:
                        data = sock.recv(RECV_SIZE)
                        if data:
                            recv_buf += data
                            while b"\n" in recv_buf:
                                _, recv_buf = recv_buf.split(b"\n", 1)
                                recv_ok += 1
                    except OSError:
                        recv_err += 1

                if mask & selectors.EVENT_WRITE:
                    num -= 1
                    try:
                        sock.sendall(payload)
                        sent_ok += 1
                    except OSError:
                        sent_err += 1

            latency = time.perf_counter() - time_point
            latencies.append(latency * 1000.0)

            sleep_left = delay - latency
            if sleep_left > 0:
                time.sleep(sleep_left)

        elapsed = time.perf_counter() - start_time
        rate = sent_ok / elapsed if elapsed > 0 else 0.0

        print(
            "sent_ok {}\nsent_err {}\nrecv_ok {}\nrecv_err {}\nelapsed {:.3f} s\nrate {:.3f} msg/s\np50 {:.3f} ms\np95 {:.3f} ms\np99 {:.3f} ms".format(
                sent_ok,
                sent_err,
                recv_ok,
                recv_err,
                elapsed,
                rate,
                percentile(latencies, 0.50),
                percentile(latencies, 0.95),
                percentile(latencies, 0.99),
            )
        )

        if args.log:
            with open(args.log, "w") as file:
                for val in latencies:
                    file.write(f"{val:.3f}\n")


if __name__ == "__main__":
    main()
