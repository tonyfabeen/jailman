#encoding: utf-8

require 'spec_helper'
require 'jailman/configuration'

describe Jailman::Configuration do
  context 'when initialized' do
    context 'and no jail passed as argument' do

      it 'raises an Exception' do
        expect{ described_class.new }.to raise_error('A jail must be passed')
      end

    end

    context 'when a jail is filled' do

      it 'populates jail' do
        jail    = Jailman::Jail.new('my_jail')
        config  = described_class.new(jail)

        expect(config.jail).to_not be_nil
        expect(config.jail).to be(jail)
      end

    end

  end
end
