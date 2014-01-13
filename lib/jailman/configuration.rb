#encoding: utf-8

module Jailman
  class Configuration

    attr_accessor :jail

    def initialize(jail=nil)
      raise ArgumentError.new('A jail must be passed') unless jail
      @jail = jail
    end

  end
end
