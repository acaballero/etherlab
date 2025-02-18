;; Project-specific emacs configuration

(message "Loading project configuration")

(defun start-openocd ()
"Start OpenOCD."
  (message "Starting OpenOCD...")
  (start-process "openocd" "*openocd*" "/home/ahcr/.platformio/packages/tool-openocd/bin/openocd" "-f" "/home/ahcr/.platformio/packages/tool-openocd/openocd/scripts/board/stm32f4discovery.cfg")
  )

(defun stop-openocd ()
  "Stop OpenOCD."
  ;; Stop dape in case it was on
  (dape-quit)
  
  "Stop OpenOCD."
   (let ((openocd-process (get-process "openocd"))) 
(when openocd-process (message "Stopping existing OpenOCD process...") 
(delete-process openocd-process)))
  )

(defun my-dap-restart-openocd-and-debug () "Stop existing OpenOCD instance, start a new one, and launch dap-debug." (interactive)
  ;; Stop any running OpenOCD processes
  (stop-openocd)

   ;; Wait a bit for OpenOCD to release ports
  (sleep-for 1)

  (start-openocd)

  ;; Wait a bit for OpenOCD to initialize
  (sleep-for 1)

  ;; Run dap-debug
  (message "Starting dap-debug...") 
  (let ((current-prefix-arg t)) 
  (call-interactively 'dap-debug-last))

  )


(defun my-start-dape () "Stop existing OpenOCD instance, start a new one, and launch dape." (interactive)
  ;; Stop any running OpenOCD processes
  (stop-openocd)

   ;; Wait a bit for OpenOCD to release ports
  (sleep-for 1)

  (start-openocd)

  ;; Wait a bit for OpenOCD to initialize
  ;;(sleep-for 1)

  ;; Run dap-debug
  (message "Starting dap-debug...") 
  (let ((current-prefix-arg t)) 
  (call-interactively 'dape))

  )


(defun my-dap-kill-openocd-and-upload () "Stop existing OpenOCD instance and run platformio upload." (interactive)
       
  ;; Stop any running OpenOCD processes
  (stop-openocd)

  ;; Wait a bit for OpenOCD to release ports
  (sleep-for 1)

  ;; Run pio upload
  (message "Starting dap-debug...") 
(call-interactively 'platformio-upload))

(defvar my-current-dir (file-name-directory (or load-file-name buffer-file-name)) "The directory of the currently loaded or evaluated .el file.")

;; Start OpenOCD
(defun start-openocd-after-compilation (buffer desc) "Start OpenOCD."
(if (string-match "exited abnormally" desc) 
(message "❌ Compilation failed!") 
(progn
  (start-openocd)
  ;; Start SWOParser
  ;; Kill the existing swoparser process if it exists
  (let ((swoparser-process (get-process "swoparser"))) 
(when swoparser-process (message "Killing existing swoparser process...") 
(delete-process swoparser-process)))
(start-process "swoparser" "*swoparser*"  "python" (concat my-current-dir "swoparser.py")) 
(display-buffer "*swoparser*")
(goto-char (point-max))
))
(remove-hook 'compilation-finish-functions 'start-openocd-after-compilation)
)

(defun my-dap-reset-openocd-and-upload () "Stop existing OpenOCD instance, run platformio\n \
upload and start OpenOCD and the SWO parser\n \
for debug echo." (interactive)
  ;; Stop any running OpenOCD processes
  (stop-openocd)

   ;; Wait a bit for OpenOCD to release ports
  (sleep-for 1)
  ;; Run pio upload
  (message "Uploading firmware...")
(add-hook 'compilation-finish-functions 'start-openocd-after-compilation) 
(call-interactively 'platformio-upload)
)


;;(global-set-key (kbd "C-c d") 'my-dap-restart-openocd-and-debug)
(global-set-key (kbd "C-c d") 'my-start-dape)
(global-set-key (kbd "C-c u") 'my-dap-kill-openocd-and-upload)
(global-set-key (kbd "C-c o") 'my-dap-reset-openocd-and-upload)


;;;; DAPE (debugger) CONFiG ;;;;;

(require 'dape)
;; Dape configs
(add-to-list 'dape-configs
	     `(gdb-multiarch
	       modes (c++-mode c-mode)
	       command "arm-none-eabi-gdb"
	       command-args ["-i" "dap" "-x" "/home/ahcr/dev/trx/gdb_init.txt"]
	       defer-launch-attach nil
	       command-cwd "/home/ahcr/dev/trx/"
	       :request "attach"
	      ; :gdbpath "arm-none-eabi-gdb"   
	       :type "gdb"
	       :cwd dape-cwd-fn
	      ;  :console "integratedTerminal" 
	       :stopOnEntry t
	      ; :externalConsole nil
              ; :targetArchitecture "arm"
               :valuesFormatting "prettyPrinters"
              ; :breakpoint-set "hbreak"
	       :showReturnValue t
	       :stopAtBeginningOfMainSubprogram t
	       :stopAtEntry t
	       :target ":3333"	      
	       :name "Debug GDB-OpenOCD"
	       :args  ["ex" "break main"]	    
	       ;:port "3333"
	       ;:host "localhost"
	       ;:program "/home/ahcr/dev/trx/.pio/build/genericSTM32F427VGT/firmware.elf"
	      ; :remote "localhost:3333"
	 
       
 ))
