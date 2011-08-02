                        FS4000US software

  This zip file contains two programs for drive a Canon FS4000US scanner.

  Fs4000.exe is the original command line program that provides the best
  control of the scanner.  It produces TIFF files but there is no display
  capability.

  FsHost.exe is a GUI front-end that can display a thmbnail and create scan
  files.  It provides auto-exposure and auto-repeat options.

  This zip file also contains all the source files for the programs.


                        Installation

  The zip file should be unzipped to an empty directory.  The text files
  (Fs4000.txt and FsHost.txt) contain operating instructions.

  The programs need ASPI to access a SCSI scanner.  For a USB scanner the
  Canon driver files must be installed so the operating system recognises the
  device as a scanner.

  The programs use the current working directory for work and output files.
  This is generally the directory containing the exe files but can be modified
  (e.g. by a shortcut for FsHost.exe) if required.  All work and output files
  have names starting with '-' (e.g. "-FsHost.log" or "-Scan005.tif").


                        Operation

  Fs4000.exe is a console app and the functions/modifiers required are
  specified by keywords in the command tail.  It produces console output
  detailing the operations performed.  The Fs4000.txt file lists the valid
  keywords and their effects.

  FsHost.exe is a GUI program that can be driven by keystrokes or mouse
  input.  The FsHost.txt file lists the valid keystrokes and details about
  this program.


                        Contact

  For bug reports or support e-mail Steven Saunderson (steven at phelum.net)

