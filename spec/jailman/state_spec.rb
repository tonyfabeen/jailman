#encoding: utf-8
require 'spec_helper'
require 'jailman/state'

describe Jailman::State do

  context 'when initialized' do

    context 'whithout a jail' do

      it 'raises an exception' do
        expect { described_class.new }.to raise_error("jail must be present")
      end

    end

    context 'with a jail' do

      it 'initializes correctly' do
        state = described_class.new(jail_factory)
        expect(state.jail).to_not be_nil
      end

    end

  end

  context '#save' do
    pending
  end

end
