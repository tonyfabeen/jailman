#encoding: utf-8

module Jailman
  class Provisioner

    attr_accessor :jail
    attr_accessor :rootfs_script

    def initialize(jail=nil)
      raise ArgumentError.new('A jail must be passed') unless jail
      @jail = jail
      @rootfs_script = "ps_rootfs"
    end

    def create_rootfs
      script = "#{rootfs_script} #{jail.name} #{jail.directory}"
      output = `#{script}`
      raise output unless $? == 0
    end

  end
end
