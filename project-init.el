;; Project-specific emacs configuration
;; Kill and start existing OpenOCD processes

(message "Loading project configuration")

(defun my-dap-restart-openocd-and-debug ()
  "Stop existing OpenOCD instance, start a new one, and launch dap-debug."
  (interactive)
  ;; Stop any running OpenOCD processes
  (let ((openocd-process (get-process "openocd")))
    (when openocd-process
      (message "Stopping existing OpenOCD process...")
      (delete-process openocd-process)))

   ;; Wait a bit for OpenOCD to release ports
  (sleep-for 1)
  
  ;; Start OpenOCD
  (message "Starting OpenOCD...")
  (start-process "openocd" "*openocd*"
		 "/home/ahcr/.platformio/packages/tool-openocd/bin/openocd" "-d0" "-f" "/home/ahcr/.platformio/packages/tool-openocd/openocd/scripts/board/stm32f4discovery.cfg")

  ;; Wait a bit for OpenOCD to initialize
  (sleep-for 1)

  ;; Run dap-debug
  (message "Starting dap-debug...")
  (let ((current-prefix-arg t))
  (call-interactively 'dap-debug-last))


  )

(defun my-dap-kill-openocd-and-upload ()
  "Stop existing OpenOCD instance and run platformio upload."
  (interactive)
  ;; Stop any running OpenOCD processes
  (let ((openocd-process (get-process "openocd")))
    (when openocd-process
      (message "Stopping existing OpenOCD process...")
      (delete-process openocd-process)))

   ;; Wait a bit for OpenOCD to release ports
  (sleep-for 1)

  ;; Run pio upload
  (message "Starting dap-debug...") 
  (call-interactively 'platformio-upload)
  )

(defvar my-current-dir
  (file-name-directory (or load-file-name buffer-file-name))
  "The directory of the currently loaded or evaluated .el file.")

;; Start OpenOCD
(defun start-openocd-after-compilation (buffer desc)
  "Start OpenOCD."
  (message "Starting OpenOCD...")
  (start-process "openocd" "*openocd*"
		"/home/ahcr/.platformio/packages/tool-openocd/bin/openocd" "-d0" "-f" "/home/ahcr/.platformio/packages/tool-openocd/openocd/scripts/board/stm32f4discovery.cfg")


  ;; Start SWOParser
 
  ;; Kill the existing swoparser process if it exists
  (let ((swoparser-process (get-process "swoparser")))
    (when swoparser-process
      (message "Killing existing swoparser process...")
      (delete-process swoparser-process)))
  
  (start-process "swoparser" "*swoparser*"  "python" (concat my-current-dir "swoparser.py"))
  (display-buffer "*swoparser*")
   
  (remove-hook 'compilation-finish-functions 'start-openocd-after-compilation)
)


(defun my-dap-reset-openocd-and-upload ()
  "Stop existing OpenOCD instance, run platformio\n \
upload and start OpenOCD and the SWO parser\n \
for debug echo."
  (interactive)
  ;; Stop any running OpenOCD processes
  (let ((openocd-process (get-process "openocd")))
    (when openocd-process
      (message "Stopping existing OpenOCD process...")
      (delete-process openocd-process)))

   ;; Wait a bit for OpenOCD to release ports
  (sleep-for 1)

 
  ;; Run pio upload
  (message "Uploading firmware...")
  
  (add-hook 'compilation-finish-functions 'start-openocd-after-compilation)  
  (call-interactively 'platformio-upload)
)


(global-set-key (kbd "C-c d") 'my-dap-restart-openocd-and-debug)
(global-set-key (kbd "C-c u") 'my-dap-kill-openocd-and-upload)
(global-set-key (kbd "C-c o") 'my-dap-reset-openocd-and-upload)

