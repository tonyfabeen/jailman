#encoding: utf-8
module Jailman
  class State

    attr_reader :jail

    def initialize(jail=nil)
      raise ArgumentError.new("jail must be present") unless jail
      @jail = jail
    end

    def save
      #TODO:
    end

  end
end
