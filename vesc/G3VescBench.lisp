; MAX G3 bench-only VESC LispBM endpoint.
; It never enables motor output. Use it to verify the ESP32 <-> VESC I2C link.
; Do not turn this into a ride script until the dashboard protocol, motor,
; battery, BMS, Hall sensors, brakes, and thermal limits are measured.

(def rx-buf (array-create 8))
(def tx-buf (array-create 8))
(def checksum-buf (array-create 2))
(def vt-good-rx 0)
(def vt-bad-rx 0)
(def vt-throttle-raw 0)
(def vt-brake-raw 0)
(def vt-output-disabled 1)

(app-disable-output 0)
(set-current-rel 0.0)

(defun checksum-write (array)
  {
    (var sum 0)
    (looprange i 0 5 (set 'sum (+ sum (bufget-u8 array i))))
    (set 'sum (bitwise-xor sum 0xffff))
    (bufset-u8 array 6 sum)
    (bufset-u8 array 7 (shr sum 8))
  })

(defun checksum-valid (array)
  {
    (var sum 0)
    (looprange i 0 5 (set 'sum (+ sum (bufget-u8 array i))))
    (bufset-u8 checksum-buf 0 (bufget-u8 array 7))
    (bufset-u8 checksum-buf 1 (bufget-u8 array 6))
    (= (+ sum (bufget-u16 checksum-buf 0)) 0xffff)
  })

(defun refresh-status ()
  {
    (var speed-encoded (abs (round (* (get-speed) 10))))
    (var battery-encoded (round (* (get-batt) 100)))
    (var motor-temp-encoded (round (+ (get-temp-mot) 50)))
    (var fet-temp-encoded (round (+ (get-temp-fet) 50)))
    (if (> speed-encoded 255) (set 'speed-encoded 255))
    (if (< battery-encoded 0) (set 'battery-encoded 0))
    (if (> battery-encoded 100) (set 'battery-encoded 100))
    (if (< motor-temp-encoded 0) (set 'motor-temp-encoded 0))
    (if (> motor-temp-encoded 255) (set 'motor-temp-encoded 255))
    (if (< fet-temp-encoded 0) (set 'fet-temp-encoded 0))
    (if (> fet-temp-encoded 255) (set 'fet-temp-encoded 255))
    (bufset-u8 tx-buf 0 speed-encoded)
    (bufset-u8 tx-buf 1 battery-encoded)
    (bufset-u8 tx-buf 2 motor-temp-encoded)
    (bufset-u8 tx-buf 3 fet-temp-encoded)
    (bufset-u8 tx-buf 4 0)
    (bufset-u8 tx-buf 5 0)
    (checksum-write tx-buf)
  })

(defun poll ()
  {
    (app-disable-output 0)
    (set-current-rel 0.0)
    (refresh-status)
    (if (= (i2c-tx-rx 0x45 tx-buf rx-buf) 1)
      (if (checksum-valid rx-buf)
        {
          (set 'vt-good-rx (+ vt-good-rx 1))
          (set 'vt-throttle-raw (bufget-u8 rx-buf 0))
          (set 'vt-brake-raw (bufget-u8 rx-buf 1))
          (set 'vt-output-disabled (bufget-u8 rx-buf 4))
        }
        (set 'vt-bad-rx (+ vt-bad-rx 1))))
    (yield 20000)
    (poll)
  })

(i2c-start 'rate-200k)
(yield 200000)
(poll)
