#!/usr/bin/perl

# Data of importance:
#   $attrs{$program_name}{$attribute} = $value

$wmconfig_dir = "/etc/X11/wmconfig";
$dt_dir = "/usr/local/share/apps";

opendir(DH, $wmconfig_dir);
@wments = readdir(DH);
closedir(DH);

# Read in /etc/X11/wmconfig entries into the %attrs hash
foreach $anent(@wments)
  {
    &process_wment($anent);
  }

# Write out the information in %attrs hash to the .desktop entries
foreach $anent(keys %attrs)
  {
    &produce_dtent($anent);
  }

sub process_wment
  {
    my($fn) = @_;
    open(FH, $wmconfig_dir."/".$fn) || return;
    
    while(chomp($aline = <FH>))
      {
	if($aline =~ /^([^\s]+)\s+([^\s]+)\s+"?([^"]*)"?$/)
	  {
	    $ent = lc $1;
	    $attr = lc $2;
	    $val = $3;
	    if($attr eq "exec") { $val =~ s/\s*&$//; }
	    $attrs{$ent}{lc $attr} = $val;
	  }
	else
	  {
	    print "Unknown line for $fn: $aline\n";
	  }
      }
    close(FH);
  }

sub produce_dtent
  {
    my($ent) = @_;
    my($thedir) = $dt_dir."/".$attrs{$ent}{"group"};
    my($thepid) = fork();
    
    if($thepid)
      {
	waitpid($thepid,0);
      }
    else
      {
	exec("/bin/mkdir","-p",$thedir);
      }

    open(FH, ">$thedir/$ent.desktop") || return;
    printf FH "[Desktop Entry]\nName=%s\nDescription=%s\nExec=%s\n",
    $attrs{$ent}{"name"}, $attrs{$ent}{"description"},
    $attrs{$ent}{"exec"};
    if($attrs{$ent}{"icon"})
      {
	printf FH "Icon=/usr/share/icons/%s\n",
	$attrs{$ent}{"icon"};
      }
    elsif($attrs{$ent}{"mini-icon"})
      {
	printf FH "Icon=/usr/share/icons/%s\n",
	$attrs{$ent}{"mini-icon"};
      }
  }
